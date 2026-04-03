/*
 * category_gtk.c  — WIGMS v3
 * GTK 4  Category Management
 *
 * Compile:
 *   gcc category_gtk.c -o category_gtk.exe $(pkg-config --cflags --libs gtk4)
 *
 * Changes v3:
 *  - "All Categories" tab is Admin-only — access code: "pokemon"
 *    It shows each category WITH its auto-generated internal ID.
 *  - New "Classify By" tab: groups guests across all categories
 *    by Name (A-Z), Age (ascending), or Social Class (A-Z).
 */

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ADMIN_CODE "pokemon"

/* ── Data types ─────────────────────────────── */
typedef enum { LE=0, LA=1 } side;
typedef struct { int id; char name[100]; int age; char social_class[50]; side side; int parking; } Person;
typedef struct { int id; char code[50]; char guest_name[4][100]; } Category;

/* ── CSV paths ───────────────────────────────── */
#define CSV_CAT  "categories.csv"
#define CSV_PER  "persons.csv"
#define HDR_CAT  "id,code,g0_name,g1_name,g2_name,g3_name\n"
#define HDR_PER  "id,name,age,social_class,side,parking\n"

static void strip_nl(char *s){size_t n=strlen(s);while(n>0&&(s[n-1]=='\n'||s[n-1]=='\r'))s[--n]='\0';}
static void parse_field(char **ptr,char *dst,int maxlen){
    char *p=*ptr;int i=0;
    if(*p=='"'){p++;while(*p&&!(*p=='"'&&(*(p+1)==','||*(p+1)=='\0'))){if(i<maxlen-1)dst[i++]=*p;p++;}if(*p=='"')p++;}
    else{while(*p&&*p!=','){if(i<maxlen-1)dst[i++]=*p;p++;}}
    dst[i]='\0';if(*p==',')p++;*ptr=p;
}

/* ── Person CSV ──────────────────────────────── */
static void csv_person_init(void){
    FILE *f=fopen(CSV_PER,"r");if(f){fclose(f);return;}
    f=fopen(CSV_PER,"w");if(f){fputs(HDR_PER,f);fclose(f);}
}
static GArray *csv_load_persons(void){
    GArray *arr=g_array_new(FALSE,TRUE,sizeof(Person));
    FILE *f=fopen(CSV_PER,"r");if(!f)return arr;
    char line[512];fgets(line,sizeof(line),f);
    while(fgets(line,sizeof(line),f)){
        strip_nl(line);if(strlen(line)<3)continue;
        Person p={0};char tmp[256];char *ptr=line;
        parse_field(&ptr,tmp,sizeof(tmp));         p.id=atoi(tmp);
        parse_field(&ptr,p.name,sizeof(p.name));
        parse_field(&ptr,tmp,sizeof(tmp));         p.age=atoi(tmp);
        parse_field(&ptr,p.social_class,sizeof(p.social_class));
        parse_field(&ptr,tmp,sizeof(tmp));         p.side=strcmp(tmp,"LE")==0?LE:LA;
        parse_field(&ptr,tmp,sizeof(tmp));         p.parking=strcmp(tmp,"YES")==0?1:0;
        g_array_append_val(arr,p);
    }
    fclose(f);return arr;
}

/* ── Category CSV ────────────────────────────── */
static void csv_cat_init(void){
    FILE *f=fopen(CSV_CAT,"r");if(f){fclose(f);return;}
    f=fopen(CSV_CAT,"w");if(f){fputs(HDR_CAT,f);fclose(f);}
}
static GArray *csv_load_cats(void){
    GArray *arr=g_array_new(FALSE,TRUE,sizeof(Category));
    FILE *f=fopen(CSV_CAT,"r");if(!f)return arr;
    char line[1024];fgets(line,sizeof(line),f);
    while(fgets(line,sizeof(line),f)){
        strip_nl(line);if(strlen(line)<2)continue;
        Category c={0};char tmp[64];char *ptr=line;
        parse_field(&ptr,tmp,sizeof(tmp));c.id=atoi(tmp);
        parse_field(&ptr,c.code,sizeof(c.code));
        for(int g=0;g<4;g++)parse_field(&ptr,c.guest_name[g],100);
        g_array_append_val(arr,c);
    }
    fclose(f);return arr;
}
static void csv_rewrite_cats(GArray *arr){
    FILE *f=fopen(CSV_CAT,"w");if(!f)return;
    fputs(HDR_CAT,f);
    for(guint i=0;i<arr->len;i++){
        Category *c=&g_array_index(arr,Category,i);
        fprintf(f,"%d,\"%s\",\"%s\",\"%s\",\"%s\",\"%s\"\n",
                c->id,c->code,c->guest_name[0],c->guest_name[1],c->guest_name[2],c->guest_name[3]);
    }
    fclose(f);
}
static int csv_cat_next_id(void){
    GArray *arr=csv_load_cats();int mx=0;
    for(guint i=0;i<arr->len;i++){int id=g_array_index(arr,Category,i).id;if(id>mx)mx=id;}
    g_array_free(arr,TRUE);return mx+1;
}
static int find_cat_by_code(GArray *arr,const char *code){
    for(guint i=0;i<arr->len;i++)
        if(strcmp(g_array_index(arr,Category,i).code,code)==0)return(int)i;
    return -1;
}

/* ── UI state ────────────────────────────────── */
typedef struct {
    /* Register */
    GtkWidget *entry_code,*combo_guest[4],*lbl_next_id,*btn_save,*lbl_status;
    /* Update */
    GtkWidget *combo_upd_code,*entry_upd_code,*combo_upd_guest[4];
    GtkWidget *btn_load,*btn_update,*lbl_upd_status;
    /* Delete */
    GtkWidget *combo_del_code,*btn_delete,*lbl_del_status;
    /* Public list */
    GtkWidget *entry_search,*tree,*lbl_count;
    GtkListStore *store;
    /* Admin list (with ID) */
    GtkWidget *admin_stack;
    GtkWidget *entry_admin_code,*lbl_admin_status;
    GtkWidget *admin_tree;
    GtkListStore *admin_store;
    GtkWidget *lbl_admin_count;
    /* Classify By tab */
    GtkWidget *combo_classify;
    GtkWidget *classify_tree;
    GtkListStore *classify_store;
} AppWidgets;

/* Public list cols */
enum { COL_CODE=0,COL_G0,COL_G1,COL_G2,COL_G3,N_COLS };
/* Admin list cols (with ID) */
enum { ACOL_ID=0,ACOL_CODE,ACOL_G0,ACOL_G1,ACOL_G2,ACOL_G3,ACOL_N };
/* Classify cols: Category | Guest | Age | Social Class */
enum { CL_CAT=0,CL_NAME,CL_AGE,CL_CLASS,CL_SIDE,CL_N };

/* ── Person list for dropdowns ───────────────── */
static GtkStringList *make_person_list(void){
    GtkStringList *sl=gtk_string_list_new(NULL);
    gtk_string_list_append(sl,"— none —");
    GArray *arr=csv_load_persons();
    for(guint i=0;i<arr->len;i++) gtk_string_list_append(sl,g_array_index(arr,Person,i).name);
    g_array_free(arr,TRUE);return sl;
}

static void repopulate_combos(AppWidgets *w){
    GtkStringList *uc=gtk_string_list_new(NULL);
    GtkStringList *dc=gtk_string_list_new(NULL);
    GArray *cats=csv_load_cats();
    for(guint i=0;i<cats->len;i++){
        const char *code=g_array_index(cats,Category,i).code;
        gtk_string_list_append(uc,code);gtk_string_list_append(dc,code);
    }
    g_array_free(cats,TRUE);
    gtk_drop_down_set_model(GTK_DROP_DOWN(w->combo_upd_code),G_LIST_MODEL(uc));
    gtk_drop_down_set_model(GTK_DROP_DOWN(w->combo_del_code),G_LIST_MODEL(dc));
    for(int g=0;g<4;g++){
        gtk_drop_down_set_model(GTK_DROP_DOWN(w->combo_guest[g]),    G_LIST_MODEL(make_person_list()));
        gtk_drop_down_set_model(GTK_DROP_DOWN(w->combo_upd_guest[g]),G_LIST_MODEL(make_person_list()));
    }
}

static const char *combo_guest_name(GtkWidget *combo){
    GObject *sel=gtk_drop_down_get_selected_item(GTK_DROP_DOWN(combo));
    if(!sel)return "";
    const char *n=gtk_string_object_get_string(GTK_STRING_OBJECT(sel));
    return strcmp(n,"— none —")==0?"":n;
}

static void combo_guest_set(GtkWidget *combo,const char *name){
    GListModel *m=gtk_drop_down_get_model(GTK_DROP_DOWN(combo));
    guint n=g_list_model_get_n_items(m);
    for(guint i=0;i<n;i++){
        GObject *obj=g_list_model_get_item(m,i);
        if(strcmp(gtk_string_object_get_string(GTK_STRING_OBJECT(obj)),name)==0){
            gtk_drop_down_set_selected(GTK_DROP_DOWN(combo),i);g_object_unref(obj);return;}
        g_object_unref(obj);
    }
    gtk_drop_down_set_selected(GTK_DROP_DOWN(combo),0);
}

/* ── Admin list refresh ──────────────────────── */
static void refresh_admin_list(AppWidgets *w){
    gtk_list_store_clear(w->admin_store);
    GArray *arr=csv_load_cats();
    for(guint i=0;i<arr->len;i++){
        Category *c=&g_array_index(arr,Category,i);
        char id_s[16];snprintf(id_s,sizeof(id_s),"%d",c->id);
        GtkTreeIter it;
        gtk_list_store_append(w->admin_store,&it);
        gtk_list_store_set(w->admin_store,&it,
            ACOL_ID,id_s,ACOL_CODE,c->code,
            ACOL_G0,strlen(c->guest_name[0])>0?c->guest_name[0]:"—",
            ACOL_G1,strlen(c->guest_name[1])>0?c->guest_name[1]:"—",
            ACOL_G2,strlen(c->guest_name[2])>0?c->guest_name[2]:"—",
            ACOL_G3,strlen(c->guest_name[3])>0?c->guest_name[3]:"—",-1);
    }
    char buf[64];snprintf(buf,sizeof(buf),"Total categories: %d",(int)arr->len);
    gtk_label_set_text(GTK_LABEL(w->lbl_admin_count),buf);
    g_array_free(arr,TRUE);
}

/* ── Public list refresh ─────────────────────── */
static void refresh_list(AppWidgets *w,const char *filter){
    gtk_list_store_clear(w->store);
    GArray *arr=csv_load_cats();
    for(guint i=0;i<arr->len;i++){
        Category *c=&g_array_index(arr,Category,i);
        if(filter&&*filter){
            gchar *lc=g_utf8_strdown(c->code,-1);
            gchar *lf=g_utf8_strdown(filter,-1);
            gboolean m=strstr(lc,lf)!=NULL;
            for(int g=0;g<4&&!m;g++){gchar *lg=g_utf8_strdown(c->guest_name[g],-1);m=strstr(lg,lf)!=NULL;g_free(lg);}
            g_free(lc);g_free(lf);if(!m)continue;
        }
        GtkTreeIter it;
        gtk_list_store_append(w->store,&it);
        gtk_list_store_set(w->store,&it,
            COL_CODE,c->code,
            COL_G0,strlen(c->guest_name[0])>0?c->guest_name[0]:"—",
            COL_G1,strlen(c->guest_name[1])>0?c->guest_name[1]:"—",
            COL_G2,strlen(c->guest_name[2])>0?c->guest_name[2]:"—",
            COL_G3,strlen(c->guest_name[3])>0?c->guest_name[3]:"—",-1);
    }
    char buf[96];
    snprintf(buf,sizeof(buf),"Total categories: %d  |  Total guest slots: %d",(int)arr->len,(int)arr->len*4);
    gtk_label_set_text(GTK_LABEL(w->lbl_count),buf);
    char idbuf[32];snprintf(idbuf,sizeof(idbuf),"Auto ID: %d",csv_cat_next_id());
    gtk_label_set_text(GTK_LABEL(w->lbl_next_id),idbuf);
    g_array_free(arr,TRUE);
    repopulate_combos(w);
}

/* ── Classify By refresh ─────────────────────── */
/* Collect every (category_code, guest_name) pair, find the guest in persons.csv,
   then sort by the chosen criterion and display */

typedef struct { char cat_code[50]; char name[100]; int age; char social_class[50]; char side_s[4]; } GuestRow;

static int cmp_name(const void *a,const void *b){ return strcmp(((GuestRow*)a)->name,((GuestRow*)b)->name); }
static int cmp_age (const void *a,const void *b){ return ((GuestRow*)a)->age - ((GuestRow*)b)->age; }
static int cmp_class(const void *a,const void *b){ return strcmp(((GuestRow*)a)->social_class,((GuestRow*)b)->social_class); }

static void refresh_classify(AppWidgets *w){
    gtk_list_store_clear(w->classify_store);

    guint sel=gtk_drop_down_get_selected(GTK_DROP_DOWN(w->combo_classify));
    /* 0=Name, 1=Age, 2=Social Class */

    GArray *cats=csv_load_cats();
    GArray *persons=csv_load_persons();

    /* Build flat list of GuestRow */
    GArray *rows=g_array_new(FALSE,TRUE,sizeof(GuestRow));
    for(guint i=0;i<cats->len;i++){
        Category *c=&g_array_index(cats,Category,i);
        for(int g=0;g<4;g++){
            if(strlen(c->guest_name[g])==0) continue;
            GuestRow row={0};
            strncpy(row.cat_code,c->code,49);
            strncpy(row.name,c->guest_name[g],99);
            /* find details in persons */
            for(guint k=0;k<persons->len;k++){
                Person *p=&g_array_index(persons,Person,k);
                if(strcmp(p->name,row.name)==0){
                    row.age=p->age;
                    strncpy(row.social_class,p->social_class,49);
                    strncpy(row.side_s,p->side==LE?"LE":"LA",3);
                    break;
                }
            }
            g_array_append_val(rows,row);
        }
    }

    /* Sort */
    if(sel==0)      g_array_sort(rows,cmp_name);
    else if(sel==1) g_array_sort(rows,cmp_age);
    else            g_array_sort(rows,cmp_class);

    /* Populate tree */
    for(guint i=0;i<rows->len;i++){
        GuestRow *r=&g_array_index(rows,GuestRow,i);
        char age_s[16];snprintf(age_s,sizeof(age_s),"%d",r->age);
        GtkTreeIter it;
        gtk_list_store_append(w->classify_store,&it);
        gtk_list_store_set(w->classify_store,&it,
            CL_CAT,r->cat_code,
            CL_NAME,r->name,
            CL_AGE,age_s,
            CL_CLASS,r->social_class,
            CL_SIDE,r->side_s,-1);
    }

    g_array_free(rows,TRUE);
    g_array_free(cats,TRUE);
    g_array_free(persons,TRUE);
}

/* ── Callbacks ───────────────────────────────── */
static void on_save_clicked(GtkButton *btn,gpointer data){
    (void)btn;AppWidgets *w=(AppWidgets*)data;
    const char *scode=gtk_editable_get_text(GTK_EDITABLE(w->entry_code));
    if(!*scode){gtk_label_set_text(GTK_LABEL(w->lbl_status),"⚠  Category code is required.");return;}
    GArray *arr=csv_load_cats();
    if(find_cat_by_code(arr,scode)>=0){
        gtk_label_set_text(GTK_LABEL(w->lbl_status),"⚠  Code already exists.");g_array_free(arr,TRUE);return;}
    g_array_free(arr,TRUE);
    Category c={0};c.id=csv_cat_next_id();strncpy(c.code,scode,49);strip_nl(c.code);
    for(int g=0;g<4;g++) strncpy(c.guest_name[g],combo_guest_name(w->combo_guest[g]),99);
    FILE *f=fopen(CSV_CAT,"a");if(!f){gtk_label_set_text(GTK_LABEL(w->lbl_status),"⚠  Cannot write CSV.");return;}
    fprintf(f,"%d,\"%s\",\"%s\",\"%s\",\"%s\",\"%s\"\n",c.id,c.code,c.guest_name[0],c.guest_name[1],c.guest_name[2],c.guest_name[3]);
    fclose(f);
    gtk_label_set_text(GTK_LABEL(w->lbl_status),"✔  Category saved.");
    gtk_editable_set_text(GTK_EDITABLE(w->entry_code),"");
    for(int g=0;g<4;g++) gtk_drop_down_set_selected(GTK_DROP_DOWN(w->combo_guest[g]),0);
    refresh_list(w,NULL);
}

static void on_load_for_update(GtkButton *btn,gpointer data){
    (void)btn;AppWidgets *w=(AppWidgets*)data;
    GObject *sel=gtk_drop_down_get_selected_item(GTK_DROP_DOWN(w->combo_upd_code));
    if(!sel){gtk_label_set_text(GTK_LABEL(w->lbl_upd_status),"⚠  No category selected.");return;}
    const char *code=gtk_string_object_get_string(GTK_STRING_OBJECT(sel));
    GArray *arr=csv_load_cats();int idx=find_cat_by_code(arr,code);
    if(idx<0){gtk_label_set_text(GTK_LABEL(w->lbl_upd_status),"⚠  Not found.");g_array_free(arr,TRUE);return;}
    Category *c=&g_array_index(arr,Category,idx);
    gtk_editable_set_text(GTK_EDITABLE(w->entry_upd_code),c->code);
    for(int g=0;g<4;g++) combo_guest_set(w->combo_upd_guest[g],c->guest_name[g]);
    gtk_label_set_text(GTK_LABEL(w->lbl_upd_status),"Loaded — edit then click Update.");
    g_array_free(arr,TRUE);
}

static void on_update_clicked(GtkButton *btn,gpointer data){
    (void)btn;AppWidgets *w=(AppWidgets*)data;
    GObject *sel=gtk_drop_down_get_selected_item(GTK_DROP_DOWN(w->combo_upd_code));
    if(!sel){gtk_label_set_text(GTK_LABEL(w->lbl_upd_status),"⚠  No category selected.");return;}
    const char *orig=gtk_string_object_get_string(GTK_STRING_OBJECT(sel));
    const char *scode=gtk_editable_get_text(GTK_EDITABLE(w->entry_upd_code));
    if(!*scode){gtk_label_set_text(GTK_LABEL(w->lbl_upd_status),"⚠  Code required.");return;}
    GArray *arr=csv_load_cats();int idx=find_cat_by_code(arr,orig);
    if(idx<0){gtk_label_set_text(GTK_LABEL(w->lbl_upd_status),"⚠  Not found.");g_array_free(arr,TRUE);return;}
    Category *c=&g_array_index(arr,Category,idx);
    strncpy(c->code,scode,49);strip_nl(c->code);
    for(int g=0;g<4;g++) strncpy(c->guest_name[g],combo_guest_name(w->combo_upd_guest[g]),99);
    csv_rewrite_cats(arr);g_array_free(arr,TRUE);
    gtk_label_set_text(GTK_LABEL(w->lbl_upd_status),"✔  Category updated.");
    refresh_list(w,NULL);
}

static void on_delete_clicked(GtkButton *btn,gpointer data){
    (void)btn;AppWidgets *w=(AppWidgets*)data;
    GObject *sel=gtk_drop_down_get_selected_item(GTK_DROP_DOWN(w->combo_del_code));
    if(!sel){gtk_label_set_text(GTK_LABEL(w->lbl_del_status),"⚠  No category selected.");return;}
    const char *code=gtk_string_object_get_string(GTK_STRING_OBJECT(sel));
    GArray *arr=csv_load_cats();int idx=find_cat_by_code(arr,code);
    if(idx<0){gtk_label_set_text(GTK_LABEL(w->lbl_del_status),"⚠  Not found.");g_array_free(arr,TRUE);return;}
    g_array_remove_index(arr,idx);csv_rewrite_cats(arr);g_array_free(arr,TRUE);
    gtk_label_set_text(GTK_LABEL(w->lbl_del_status),"✔  Category deleted.");
    refresh_list(w,NULL);
}

static void on_search_changed(GtkEditable *e,gpointer data){refresh_list((AppWidgets*)data,gtk_editable_get_text(e));}

static void on_classify_changed(GtkDropDown *dd,GParamSpec *p,gpointer data){
    (void)dd;(void)p;refresh_classify((AppWidgets*)data);
}

/* Admin unlock/lock */
static void on_admin_unlock(GtkButton *btn,gpointer data){
    (void)btn;AppWidgets *w=(AppWidgets*)data;
    const char *code=gtk_editable_get_text(GTK_EDITABLE(w->entry_admin_code));
    if(strcmp(code,ADMIN_CODE)==0){
        gtk_stack_set_visible_child_name(GTK_STACK(w->admin_stack),"list");
        refresh_admin_list(w);
        gtk_editable_set_text(GTK_EDITABLE(w->entry_admin_code),"");
    } else {
        gtk_label_set_text(GTK_LABEL(w->lbl_admin_status),"⚠  Incorrect access code.");
    }
}
static void on_admin_lock(GtkButton *btn,gpointer data){
    (void)btn;AppWidgets *w=(AppWidgets*)data;
    gtk_stack_set_visible_child_name(GTK_STACK(w->admin_stack),"lock");
}

/* ── Build tabs ──────────────────────────────── */
static GtkWidget *build_register_tab(AppWidgets *w){
    GtkWidget *grid=gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid),10);gtk_grid_set_column_spacing(GTK_GRID(grid),12);
    gtk_widget_set_margin_start(grid,20);gtk_widget_set_margin_end(grid,20);
    gtk_widget_set_margin_top(grid,20);gtk_widget_set_margin_bottom(grid,20);
    int row=0;
#define AR(l,ww) gtk_grid_attach(GTK_GRID(grid),gtk_label_new(l),0,row,1,1);\
    gtk_widget_set_halign(gtk_grid_get_child_at(GTK_GRID(grid),0,row),GTK_ALIGN_START);\
    gtk_grid_attach(GTK_GRID(grid),ww,1,row,2,1);gtk_widget_set_hexpand(ww,TRUE);row++;

    w->lbl_next_id=gtk_label_new("—");gtk_widget_set_halign(w->lbl_next_id,GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid),gtk_label_new("ID (auto)"),0,row,1,1);
    gtk_widget_set_halign(gtk_grid_get_child_at(GTK_GRID(grid),0,row),GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid),w->lbl_next_id,1,row,2,1);row++;

    w->entry_code=gtk_entry_new();gtk_entry_set_placeholder_text(GTK_ENTRY(w->entry_code),"e.g. VIP-A");
    AR("Category Code",w->entry_code)

    char gl[32];
    for(int g=0;g<4;g++){
        w->combo_guest[g]=gtk_drop_down_new(G_LIST_MODEL(make_person_list()),NULL);
        gtk_widget_set_hexpand(w->combo_guest[g],TRUE);
        snprintf(gl,sizeof(gl),"Guest %d",g+1);
        gtk_grid_attach(GTK_GRID(grid),gtk_label_new(gl),0,row,1,1);
        gtk_widget_set_halign(gtk_grid_get_child_at(GTK_GRID(grid),0,row),GTK_ALIGN_START);
        gtk_grid_attach(GTK_GRID(grid),w->combo_guest[g],1,row,2,1);row++;
    }
    w->btn_save=gtk_button_new_with_label("💾  Save Category");
    gtk_grid_attach(GTK_GRID(grid),w->btn_save,0,row,3,1);row++;
    w->lbl_status=gtk_label_new("");
    gtk_grid_attach(GTK_GRID(grid),w->lbl_status,0,row,3,1);
    g_signal_connect(w->btn_save,"clicked",G_CALLBACK(on_save_clicked),w);
#undef AR
    return grid;
}

static GtkWidget *build_update_tab(AppWidgets *w){
    GtkWidget *grid=gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid),10);gtk_grid_set_column_spacing(GTK_GRID(grid),12);
    gtk_widget_set_margin_start(grid,20);gtk_widget_set_margin_end(grid,20);
    gtk_widget_set_margin_top(grid,20);gtk_widget_set_margin_bottom(grid,20);
    int row=0;
    w->combo_upd_code=gtk_drop_down_new(NULL,NULL);gtk_widget_set_hexpand(w->combo_upd_code,TRUE);
    w->btn_load=gtk_button_new_with_label("🔍  Load");
    gtk_grid_attach(GTK_GRID(grid),gtk_label_new("Select category"),0,row,1,1);
    gtk_widget_set_halign(gtk_grid_get_child_at(GTK_GRID(grid),0,row),GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid),w->combo_upd_code,1,row,1,1);
    gtk_grid_attach(GTK_GRID(grid),w->btn_load,2,row,1,1);row++;
    w->entry_upd_code=gtk_entry_new();gtk_entry_set_placeholder_text(GTK_ENTRY(w->entry_upd_code),"New code");
    gtk_widget_set_hexpand(w->entry_upd_code,TRUE);
    gtk_grid_attach(GTK_GRID(grid),gtk_label_new("Code"),0,row,1,1);
    gtk_widget_set_halign(gtk_grid_get_child_at(GTK_GRID(grid),0,row),GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid),w->entry_upd_code,1,row,2,1);row++;
    char gl[32];
    for(int g=0;g<4;g++){
        w->combo_upd_guest[g]=gtk_drop_down_new(G_LIST_MODEL(make_person_list()),NULL);
        gtk_widget_set_hexpand(w->combo_upd_guest[g],TRUE);
        snprintf(gl,sizeof(gl),"Guest %d",g+1);
        gtk_grid_attach(GTK_GRID(grid),gtk_label_new(gl),0,row,1,1);
        gtk_widget_set_halign(gtk_grid_get_child_at(GTK_GRID(grid),0,row),GTK_ALIGN_START);
        gtk_grid_attach(GTK_GRID(grid),w->combo_upd_guest[g],1,row,2,1);row++;
    }
    w->btn_update=gtk_button_new_with_label("✏️  Update Category");
    gtk_grid_attach(GTK_GRID(grid),w->btn_update,0,row,3,1);row++;
    w->lbl_upd_status=gtk_label_new("");
    gtk_grid_attach(GTK_GRID(grid),w->lbl_upd_status,0,row,3,1);
    g_signal_connect(w->btn_load,"clicked",G_CALLBACK(on_load_for_update),w);
    g_signal_connect(w->btn_update,"clicked",G_CALLBACK(on_update_clicked),w);
    return grid;
}

static GtkWidget *build_delete_tab(AppWidgets *w){
    GtkWidget *grid=gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid),10);gtk_grid_set_column_spacing(GTK_GRID(grid),12);
    gtk_widget_set_margin_start(grid,20);gtk_widget_set_margin_end(grid,20);
    gtk_widget_set_margin_top(grid,20);gtk_widget_set_margin_bottom(grid,20);
    int row=0;
    w->combo_del_code=gtk_drop_down_new(NULL,NULL);gtk_widget_set_hexpand(w->combo_del_code,TRUE);
    gtk_grid_attach(GTK_GRID(grid),gtk_label_new("Select category"),0,row,1,1);
    gtk_widget_set_halign(gtk_grid_get_child_at(GTK_GRID(grid),0,row),GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid),w->combo_del_code,1,row,2,1);row++;
    w->btn_delete=gtk_button_new_with_label("🗑  Delete Category");
    gtk_grid_attach(GTK_GRID(grid),w->btn_delete,0,row,3,1);row++;
    w->lbl_del_status=gtk_label_new("");
    gtk_grid_attach(GTK_GRID(grid),w->lbl_del_status,0,row,3,1);
    g_signal_connect(w->btn_delete,"clicked",G_CALLBACK(on_delete_clicked),w);
    return grid;
}

static GtkWidget *build_list_tab(AppWidgets *w){
    GtkWidget *vbox=gtk_box_new(GTK_ORIENTATION_VERTICAL,8);
    gtk_widget_set_margin_start(vbox,16);gtk_widget_set_margin_end(vbox,16);
    gtk_widget_set_margin_top(vbox,16);gtk_widget_set_margin_bottom(vbox,16);
    w->entry_search=gtk_search_entry_new();
    gtk_search_entry_set_placeholder_text(GTK_SEARCH_ENTRY(w->entry_search),"Search by code or guest name…");
    gtk_box_append(GTK_BOX(vbox),w->entry_search);
    g_signal_connect(w->entry_search,"search-changed",G_CALLBACK(on_search_changed),w);
    w->lbl_count=gtk_label_new("Total categories: 0");
    gtk_widget_set_halign(w->lbl_count,GTK_ALIGN_START);gtk_box_append(GTK_BOX(vbox),w->lbl_count);
    w->store=gtk_list_store_new(N_COLS,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING);
    w->tree=gtk_tree_view_new_with_model(GTK_TREE_MODEL(w->store));
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(w->tree),TRUE);
    const char *titles[N_COLS]={"Code","Guest 1","Guest 2","Guest 3","Guest 4"};
    for(int c=0;c<N_COLS;c++){
        GtkCellRenderer *rend=gtk_cell_renderer_text_new();
        GtkTreeViewColumn *col=gtk_tree_view_column_new_with_attributes(titles[c],rend,"text",c,NULL);
        gtk_tree_view_column_set_resizable(col,TRUE);gtk_tree_view_column_set_sort_column_id(col,c);
        gtk_tree_view_append_column(GTK_TREE_VIEW(w->tree),col);
    }
    GtkWidget *scroll=gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(scroll,TRUE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll),w->tree);
    gtk_box_append(GTK_BOX(vbox),scroll);
    return vbox;
}

/* Admin-only tab with ID — access code "pokemon" */
static GtkWidget *build_admin_list_tab(AppWidgets *w){
    w->admin_stack=gtk_stack_new();

    /* Lock page */
    GtkWidget *lb=gtk_box_new(GTK_ORIENTATION_VERTICAL,16);
    gtk_widget_set_margin_start(lb,60);gtk_widget_set_margin_end(lb,60);
    gtk_widget_set_margin_top(lb,60);gtk_widget_set_margin_bottom(lb,60);
    GtkWidget *lt=gtk_label_new("🔒  Admin Access");
    PangoAttrList *a=pango_attr_list_new();
    pango_attr_list_insert(a,pango_attr_weight_new(PANGO_WEIGHT_BOLD));
    pango_attr_list_insert(a,pango_attr_scale_new(1.4));
    gtk_label_set_attributes(GTK_LABEL(lt),a);pango_attr_list_unref(a);
    gtk_widget_set_halign(lt,GTK_ALIGN_CENTER);gtk_box_append(GTK_BOX(lb),lt);
    GtkWidget *lh=gtk_label_new("This section is reserved for the Administrator.\nEnter the access code to continue.");
    gtk_label_set_justify(GTK_LABEL(lh),GTK_JUSTIFY_CENTER);gtk_box_append(GTK_BOX(lb),lh);
    w->entry_admin_code=gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(w->entry_admin_code),"Access code");
    gtk_entry_set_visibility(GTK_ENTRY(w->entry_admin_code),FALSE);
    gtk_entry_set_input_purpose(GTK_ENTRY(w->entry_admin_code),GTK_INPUT_PURPOSE_PASSWORD);
    gtk_box_append(GTK_BOX(lb),w->entry_admin_code);
    GtkWidget *bu=gtk_button_new_with_label("Unlock");gtk_box_append(GTK_BOX(lb),bu);
    w->lbl_admin_status=gtk_label_new("");gtk_box_append(GTK_BOX(lb),w->lbl_admin_status);
    g_signal_connect(bu,"clicked",G_CALLBACK(on_admin_unlock),w);
    g_signal_connect_swapped(w->entry_admin_code,"activate",G_CALLBACK(on_admin_unlock),w);
    gtk_stack_add_named(GTK_STACK(w->admin_stack),lb,"lock");

    /* List page */
    GtkWidget *lv=gtk_box_new(GTK_ORIENTATION_VERTICAL,8);
    gtk_widget_set_margin_start(lv,16);gtk_widget_set_margin_end(lv,16);
    gtk_widget_set_margin_top(lv,16);gtk_widget_set_margin_bottom(lv,16);
    GtkWidget *tr=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,8);
    w->lbl_admin_count=gtk_label_new("Total: 0");
    gtk_widget_set_halign(w->lbl_admin_count,GTK_ALIGN_START);gtk_widget_set_hexpand(w->lbl_admin_count,TRUE);
    gtk_box_append(GTK_BOX(tr),w->lbl_admin_count);
    GtkWidget *bl=gtk_button_new_with_label("🔒  Lock");
    g_signal_connect(bl,"clicked",G_CALLBACK(on_admin_lock),w);
    gtk_box_append(GTK_BOX(tr),bl);gtk_box_append(GTK_BOX(lv),tr);
    w->admin_store=gtk_list_store_new(ACOL_N,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING);
    w->admin_tree=gtk_tree_view_new_with_model(GTK_TREE_MODEL(w->admin_store));
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(w->admin_tree),TRUE);
    const char *atitles[ACOL_N]={"ID","Code","Guest 1","Guest 2","Guest 3","Guest 4"};
    for(int c=0;c<ACOL_N;c++){
        GtkCellRenderer *rend=gtk_cell_renderer_text_new();
        GtkTreeViewColumn *col=gtk_tree_view_column_new_with_attributes(atitles[c],rend,"text",c,NULL);
        gtk_tree_view_column_set_resizable(col,TRUE);gtk_tree_view_column_set_sort_column_id(col,c);
        gtk_tree_view_append_column(GTK_TREE_VIEW(w->admin_tree),col);
    }
    GtkWidget *scroll=gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(scroll,TRUE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll),w->admin_tree);
    gtk_box_append(GTK_BOX(lv),scroll);
    gtk_stack_add_named(GTK_STACK(w->admin_stack),lv,"list");
    gtk_stack_set_visible_child_name(GTK_STACK(w->admin_stack),"lock");
    return w->admin_stack;
}

/* Classify By tab */
static GtkWidget *build_classify_tab(AppWidgets *w){
    GtkWidget *vbox=gtk_box_new(GTK_ORIENTATION_VERTICAL,10);
    gtk_widget_set_margin_start(vbox,16);gtk_widget_set_margin_end(vbox,16);
    gtk_widget_set_margin_top(vbox,16);gtk_widget_set_margin_bottom(vbox,16);

    /* Criterion selector */
    GtkWidget *top=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,10);
    gtk_box_append(GTK_BOX(top),gtk_label_new("Group by:"));
    const char *criteria[]={"Name (A–Z)","Age (youngest first)","Social Class (A–Z)",NULL};
    w->combo_classify=gtk_drop_down_new_from_strings(criteria);
    gtk_widget_set_hexpand(w->combo_classify,TRUE);
    gtk_box_append(GTK_BOX(top),w->combo_classify);
    gtk_box_append(GTK_BOX(vbox),top);

    /* Tree view */
    w->classify_store=gtk_list_store_new(CL_N,
        G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING);
    w->classify_tree=gtk_tree_view_new_with_model(GTK_TREE_MODEL(w->classify_store));
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(w->classify_tree),TRUE);
    const char *ct[CL_N]={"Category","Name","Age","Social Class","Side"};
    for(int c=0;c<CL_N;c++){
        GtkCellRenderer *rend=gtk_cell_renderer_text_new();
        GtkTreeViewColumn *col=gtk_tree_view_column_new_with_attributes(ct[c],rend,"text",c,NULL);
        gtk_tree_view_column_set_resizable(col,TRUE);
        gtk_tree_view_append_column(GTK_TREE_VIEW(w->classify_tree),col);
    }
    GtkWidget *scroll=gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(scroll,TRUE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll),w->classify_tree);
    gtk_box_append(GTK_BOX(vbox),scroll);

    g_signal_connect(w->combo_classify,"notify::selected",G_CALLBACK(on_classify_changed),w);
    return vbox;
}

static void activate(GtkApplication *app,gpointer user_data){
    (void)user_data;
    csv_person_init();csv_cat_init();
    AppWidgets *w=g_new0(AppWidgets,1);
    GtkWidget *window=gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window),"WIGMS — Category Management");
    gtk_window_set_default_size(GTK_WINDOW(window),860,580);
    GtkWidget *nb=gtk_notebook_new();
    gtk_notebook_append_page(GTK_NOTEBOOK(nb),build_register_tab(w), gtk_label_new("📝  Register"));
    gtk_notebook_append_page(GTK_NOTEBOOK(nb),build_update_tab(w),   gtk_label_new("✏️  Update"));
    gtk_notebook_append_page(GTK_NOTEBOOK(nb),build_delete_tab(w),   gtk_label_new("🗑  Delete"));
    gtk_notebook_append_page(GTK_NOTEBOOK(nb),build_list_tab(w),     gtk_label_new("📋  Categories"));
    gtk_notebook_append_page(GTK_NOTEBOOK(nb),build_admin_list_tab(w),gtk_label_new("🔒  All Categories"));
    gtk_notebook_append_page(GTK_NOTEBOOK(nb),build_classify_tab(w), gtk_label_new("📊  Classify By"));
    gtk_window_set_child(GTK_WINDOW(window),nb);
    refresh_list(w,NULL);
    refresh_classify(w);
    gtk_widget_set_visible(window,TRUE);
}

int main(int argc,char **argv){
    GtkApplication *app=gtk_application_new("cm.wigms.category",G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app,"activate",G_CALLBACK(activate),NULL);
    int s=g_application_run(G_APPLICATION(app),argc,argv);
    g_object_unref(app);return s;
}
