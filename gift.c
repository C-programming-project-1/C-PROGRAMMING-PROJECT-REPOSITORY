/*
 * gift.c - WIGMS
 * Gift Management Module (Console Version)
 *
 * Compile (standalone test):
 *   gcc gift.c -o gift.exe -lm
 *
 * When integrating with main WIGMS system:
 *   Include gift.h and call gift_main_menu() from the home console.
 */

/* ── Standard library headers ── */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ── Constants ── */
#define ADMIN_PASSWORD  "pokemon"
#define GIFTS_CSV       "gifts.csv"
#define ORDERS_CSV      "gift_orders.csv"
#define MAX_GIFTS       100
#define MAX_ORDERS      500

/* ── Data types ── */
typedef enum { AVAILABLE = 0, RESERVED = 1 } GiftStatus;

typedef struct {
    int        gift_id;
    char       name[100];
    float      price;
    char       description[200];
    GiftStatus status;
} Gift;

typedef struct {
    Gift items[MAX_GIFTS];
    int  count;
} GiftList;

typedef struct {
    int   order_id;
    int   guest_id;
    char  guest_name[100];
    int   gift_id;
    char  gift_name[100];
    float gift_price;
} GiftOrder;

typedef struct {
    GiftOrder items[MAX_ORDERS];
    int       count;
} OrderList;

/* ── Forward declarations ── */
void  strip_newline(char *s);
void  load_gifts(GiftList *list);
void  save_gifts(const GiftList *list, const OrderList *orders);
void  display_gifts(const GiftList *list);
void  display_all_orders(const OrderList *orders);
void  display_orders_by_frequency(const OrderList *orders, const GiftList *list);
float total_gift_value(const OrderList *orders);
void  add_gift(GiftList *list);
void  remove_gift(GiftList *list, int gift_id);
void  update_gifts(GiftList *list);
float compute_user_total(const OrderList *orders, int guest_id);
void  display_user_orders(const OrderList *orders, int guest_id);
void  choose_gifts(GiftList *list, OrderList *orders, int guest_id, const char *guest_name);
void  gift_admin_menu(GiftList *list, OrderList *orders);
void  gift_user_menu(GiftList *list, OrderList *orders);
void  gift_main_menu(GiftList *list, OrderList *orders);
int   gift_exists(const GiftList *list, int gift_id);
int   already_ordered(const OrderList *orders, int guest_id, int gift_id);
int   next_order_id(const OrderList *orders);
int   next_gift_id(const GiftList *list);

/* ===============================================
 *  UTILITY HELPERS
 * =============================================== */

void strip_newline(char *s) {
    size_t n = strlen(s);
    while (n > 0 && (s[n-1] == '\n' || s[n-1] == '\r')) s[--n] = '\0';
}

static void clear_screen(void) {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

static void press_enter(void) {
    printf("\n  Press [Enter] to continue...");
    getchar();
}

static void print_separator(void) {
    printf("  ------------------------------------------------------------\n");
}

static void print_title(const char *title) {
    printf("\n  ============================================================\n");
    printf("  %s\n", title);
    printf("  ============================================================\n");
}

int gift_exists(const GiftList *list, int gift_id) {
    for (int i = 0; i < list->count; i++)
        if (list->items[i].gift_id == gift_id) return i;
    return -1;
}

int already_ordered(const OrderList *orders, int guest_id, int gift_id) {
    for (int i = 0; i < orders->count; i++)
        if (orders->items[i].guest_id == guest_id &&
            orders->items[i].gift_id  == gift_id)   return 1;
    return 0;
}

int next_order_id(const OrderList *orders) {
    int mx = 0;
    for (int i = 0; i < orders->count; i++)
        if (orders->items[i].order_id > mx) mx = orders->items[i].order_id;
    return mx + 1;
}

int next_gift_id(const GiftList *list) {
    int mx = 0;
    for (int i = 0; i < list->count; i++)
        if (list->items[i].gift_id > mx) mx = list->items[i].gift_id;
    return mx + 1;
}

/* ===============================================
 *  CSV PERSISTENCE
 * =============================================== */

void load_gifts(GiftList *list) {
    list->count = 0;
    FILE *f = fopen(GIFTS_CSV, "r");
    if (!f) return;

    char line[512];
    fgets(line, sizeof(line), f); /* skip header */

    while (fgets(line, sizeof(line), f) && list->count < MAX_GIFTS) {
        strip_newline(line);
        if (strlen(line) < 3) continue;

        Gift g = {0};
        char status_str[20] = {0};

        /* Parse: gift_id,name,price,description,status */
        char *tok = strtok(line, ",");
        if (!tok) continue; g.gift_id = atoi(tok);

        tok = strtok(NULL, ",");
        if (!tok) continue;
        /* handle quoted name */
        if (tok[0] == '"') {
            /* remove leading quote */
            memmove(tok, tok + 1, strlen(tok));
            /* find closing quote - may be in same or next token */
            char *end = strchr(tok, '"');
            if (end) *end = '\0';
        }
        strncpy(g.name, tok, sizeof(g.name)-1);

        tok = strtok(NULL, ",");
        if (!tok) continue; g.price = (float)atof(tok);

        tok = strtok(NULL, ",");
        if (!tok) continue;
        if (tok[0] == '"') {
            memmove(tok, tok + 1, strlen(tok));
            char *end = strchr(tok, '"');
            if (end) *end = '\0';
        }
        strncpy(g.description, tok, sizeof(g.description)-1);

        tok = strtok(NULL, ",");
        if (tok) {
            strip_newline(tok);
            g.status = (strcmp(tok, "RESERVED") == 0) ? RESERVED : AVAILABLE;
        }

        list->items[list->count++] = g;
    }
    fclose(f);
}

static void load_orders(OrderList *orders) {
    orders->count = 0;
    FILE *f = fopen(ORDERS_CSV, "r");
    if (!f) return;

    char line[512];
    fgets(line, sizeof(line), f); /* skip header */

    while (fgets(line, sizeof(line), f) && orders->count < MAX_ORDERS) {
        strip_newline(line);
        if (strlen(line) < 3) continue;

        GiftOrder o = {0};
        char *tok = strtok(line, ",");
        if (!tok) continue; o.order_id = atoi(tok);

        tok = strtok(NULL, ",");
        if (!tok) continue; o.guest_id = atoi(tok);

        tok = strtok(NULL, ",");
        if (!tok) continue;
        if (tok[0] == '"') { memmove(tok, tok+1, strlen(tok)); char *e=strchr(tok,'"'); if(e)*e='\0'; }
        strncpy(o.guest_name, tok, sizeof(o.guest_name)-1);

        tok = strtok(NULL, ",");
        if (!tok) continue; o.gift_id = atoi(tok);

        tok = strtok(NULL, ",");
        if (!tok) continue;
        if (tok[0] == '"') { memmove(tok, tok+1, strlen(tok)); char *e=strchr(tok,'"'); if(e)*e='\0'; }
        strncpy(o.gift_name, tok, sizeof(o.gift_name)-1);

        tok = strtok(NULL, ",");
        if (tok) { strip_newline(tok); o.gift_price = (float)atof(tok); }

        orders->items[orders->count++] = o;
    }
    fclose(f);
}

void save_gifts(const GiftList *list, const OrderList *orders) {
    /* Save gift catalogue */
    FILE *f = fopen(GIFTS_CSV, "w");
    if (f) {
        fprintf(f, "gift_id,name,price,description,status\n");
        for (int i = 0; i < list->count; i++) {
            const Gift *g = &list->items[i];
            fprintf(f, "%d,\"%s\",%.2f,\"%s\",%s\n",
                    g->gift_id, g->name, g->price, g->description,
                    g->status == RESERVED ? "RESERVED" : "AVAILABLE");
        }
        fclose(f);
    }

    /* Save orders */
    FILE *fo = fopen(ORDERS_CSV, "w");
    if (fo) {
        fprintf(fo, "order_id,guest_id,guest_name,gift_id,gift_name,gift_price\n");
        for (int i = 0; i < orders->count; i++) {
            const GiftOrder *o = &orders->items[i];
            fprintf(fo, "%d,%d,\"%s\",%d,\"%s\",%.2f\n",
                    o->order_id, o->guest_id, o->guest_name,
                    o->gift_id, o->gift_name, o->gift_price);
        }
        fclose(fo);
    }

    printf("\n  [OK]  Data saved to %s and %s.\n", GIFTS_CSV, ORDERS_CSV);
}

/* ===============================================
 *  DISPLAY FUNCTIONS
 * =============================================== */

void display_gifts(const GiftList *list) {
    print_title("Gift Catalogue");
    if (list->count == 0) {
        printf("  (No gifts loaded. Admin must load a gift list first.)\n");
        return;
    }
    printf("  %-5s %-30s %10s  %-12s  %s\n",
           "ID", "Name", "Price(XAF)", "Status", "Description");
    print_separator();
    for (int i = 0; i < list->count; i++) {
        const Gift *g = &list->items[i];
        printf("  %-5d %-30s %10.0f  %-12s  %s\n",
               g->gift_id, g->name, g->price,
               g->status == RESERVED ? "Reserved" : "Available",
               g->description);
    }
    print_separator();
    printf("  Total gifts in catalogue: %d\n", list->count);
}

void display_all_orders(const OrderList *orders) {
    print_title("All Gift Reservations");
    if (orders->count == 0) {
        printf("  (No reservations yet.)\n");
        return;
    }
    printf("  %-6s %-25s %-30s %12s\n",
           "OrdID", "Guest Name", "Gift", "Price(XAF)");
    print_separator();
    for (int i = 0; i < orders->count; i++) {
        const GiftOrder *o = &orders->items[i];
        printf("  %-6d %-25s %-30s %12.0f\n",
               o->order_id, o->guest_name, o->gift_name, o->gift_price);
    }
    print_separator();
    printf("  Total reservations : %d\n", orders->count);
    printf("  Total value        : %.2f XAF\n", total_gift_value(orders));
}

void display_orders_by_frequency(const OrderList *orders, const GiftList *list) {
    if (list->count == 0) { printf("  (No gifts loaded.)\n"); return; }

    /* Count reservations per gift_id */
    int counts[MAX_GIFTS]    = {0};
    int gift_ids[MAX_GIFTS]  = {0};
    int n = list->count;

    for (int i = 0; i < n; i++) gift_ids[i] = list->items[i].gift_id;
    for (int i = 0; i < orders->count; i++) {
        for (int j = 0; j < n; j++) {
            if (gift_ids[j] == orders->items[i].gift_id) { counts[j]++; break; }
        }
    }

    /* Bubble sort descending by count (small n, simple sort is fine) */
    for (int i = 0; i < n-1; i++)
        for (int j = i+1; j < n; j++)
            if (counts[j] > counts[i]) {
                int tmp_c = counts[i]; counts[i] = counts[j]; counts[j] = tmp_c;
                int tmp_id = gift_ids[i]; gift_ids[i] = gift_ids[j]; gift_ids[j] = tmp_id;
            }

    print_title("Reservations Ranked by Popularity");
    printf("  %-5s %-30s %12s\n", "ID", "Gift Name", "Reservations");
    print_separator();
    for (int i = 0; i < n; i++) {
        int idx = gift_exists(list, gift_ids[i]);
        if (idx < 0) continue;
        printf("  %-5d %-30s %12d\n",
               gift_ids[i], list->items[idx].name, counts[i]);
    }
    print_separator();
}

float total_gift_value(const OrderList *orders) {
    float total = 0.0f;
    for (int i = 0; i < orders->count; i++)
        total += orders->items[i].gift_price;
    return total;
}

/* ===============================================
 *  ADMIN - UPDATE / ADD / REMOVE
 * =============================================== */

void add_gift(GiftList *list) {
    if (list->count >= MAX_GIFTS) {
        printf("  [!]  Gift catalogue is full (%d max).\n", MAX_GIFTS);
        return;
    }
    Gift g = {0};
    g.gift_id = next_gift_id(list);
    g.status  = AVAILABLE;

    printf("\n  -- Add New Gift (ID auto: %d) --\n", g.gift_id);

    printf("  Gift name        : ");
    fgets(g.name, sizeof(g.name), stdin);
    strip_newline(g.name);

    printf("  Price (XAF)      : ");
    scanf("%f", &g.price); getchar();

    printf("  Short description: ");
    fgets(g.description, sizeof(g.description), stdin);
    strip_newline(g.description);

    list->items[list->count++] = g;
    printf("  [OK]  Gift \"%s\" added (ID %d).\n", g.name, g.gift_id);
}

void remove_gift(GiftList *list, int gift_id) {
    int idx = gift_exists(list, gift_id);
    if (idx < 0) {
        printf("  [!]  Gift ID %d not found.\n", gift_id);
        return;
    }
    char removed_name[100];
    strncpy(removed_name, list->items[idx].name, sizeof(removed_name)-1);

    /* Shift array */
    for (int i = idx; i < list->count - 1; i++)
        list->items[i] = list->items[i+1];
    list->count--;

    printf("  [OK]  Gift \"%s\" removed from catalogue.\n", removed_name);
}

void update_gifts(GiftList *list) {
    int running = 1;
    while (running) {
        clear_screen();
        print_title("Update Gift Catalogue");
        display_gifts(list);
        printf("\n  1. Add a new gift\n");
        printf("  2. Remove a gift\n");
        printf("  3. Edit gift name / price / description\n");
        printf("  0. Back\n");
        printf("  Choice: ");

        int choice;
        if (scanf("%d", &choice) != 1) { getchar(); continue; }
        getchar();

        switch (choice) {
            case 1:
                add_gift(list);
                press_enter();
                break;
            case 2: {
                printf("  Gift ID to remove: ");
                int id; scanf("%d", &id); getchar();
                remove_gift(list, id);
                press_enter();
                break;
            }
            case 3: {
                printf("  Gift ID to edit  : ");
                int id; scanf("%d", &id); getchar();
                int idx = gift_exists(list, id);
                if (idx < 0) { printf("  [!]  Gift not found.\n"); press_enter(); break; }
                Gift *g = &list->items[idx];
                printf("  New name  (leave blank to keep \"%s\"): ", g->name);
                char buf[100]; fgets(buf, sizeof(buf), stdin); strip_newline(buf);
                if (strlen(buf) > 0) strncpy(g->name, buf, sizeof(g->name)-1);
                printf("  New price (0 to keep %.2f): ", g->price);
                float p; scanf("%f", &p); getchar();
                if (p > 0) g->price = p;
                printf("  New description (blank to keep): ");
                fgets(buf, sizeof(buf), stdin); strip_newline(buf);
                if (strlen(buf) > 0) strncpy(g->description, buf, sizeof(g->description)-1);
                printf("  [OK]  Gift updated.\n");
                press_enter();
                break;
            }
            case 0:
                running = 0;
                break;
            default:
                printf("  [!]  Invalid option.\n");
                press_enter();
        }
    }
}

/* ===============================================
 *  USER - CHOOSE GIFTS
 * =============================================== */

void display_user_orders(const OrderList *orders, int guest_id) {
    printf("\n  Your current gift selections:\n");
    int found = 0;
    float total = 0.0f;
    for (int i = 0; i < orders->count; i++) {
        const GiftOrder *o = &orders->items[i];
        if (o->guest_id == guest_id) {
            printf("    [%d] %-30s  %.2f XAF\n", o->gift_id, o->gift_name, o->gift_price);
            total += o->gift_price;
            found = 1;
        }
    }
    if (!found) printf("    (none selected yet)\n");
    else {
        printf("    ------------------------------------------------------------\n");
        printf("    Subtotal: %.2f XAF\n", total);
    }
}

float compute_user_total(const OrderList *orders, int guest_id) {
    float total = 0.0f;
    for (int i = 0; i < orders->count; i++)
        if (orders->items[i].guest_id == guest_id)
            total += orders->items[i].gift_price;
    return total;
}

void choose_gifts(GiftList *list, OrderList *orders, int guest_id, const char *guest_name) {
    int running = 1;
    while (running) {
        clear_screen();
        printf("\n  Gift Selection -- Welcome, %s\n", guest_name);
        print_separator();
        display_gifts(list);
        display_user_orders(orders, guest_id);

        printf("\n  1. Add a gift to my selection\n");
        printf("  2. Remove a gift from my selection\n");
        printf("  3. Validate my selection\n");
        printf("  0. Cancel and go back\n");
        printf("  Choice: ");

        int choice;
        if (scanf("%d", &choice) != 1) { getchar(); continue; }
        getchar();

        switch (choice) {
            case 1: {
                printf("  Enter the Gift ID you want to select: ");
                int id; scanf("%d", &id); getchar();
                int idx = gift_exists(list, id);
                if (idx < 0) {
                    printf("  [!]  Gift ID %d does not exist.\n", id);
                    press_enter(); break;
                }
                if (already_ordered(orders, guest_id, id)) {
                    printf("  [!]  You already selected that gift.\n");
                    press_enter(); break;
                }
                if (orders->count >= MAX_ORDERS) {
                    printf("  [!]  Order list is full.\n");
                    press_enter(); break;
                }
                GiftOrder o = {0};
                o.order_id  = next_order_id(orders);
                o.guest_id  = guest_id;
                strncpy(o.guest_name, guest_name, sizeof(o.guest_name)-1);
                o.gift_id   = id;
                strncpy(o.gift_name, list->items[idx].name, sizeof(o.gift_name)-1);
                o.gift_price = list->items[idx].price;
                orders->items[orders->count++] = o;

                printf("  [OK]  \"%s\" added to your selection.\n", list->items[idx].name);
                press_enter();
                break;
            }
            case 2: {
                printf("  Enter the Gift ID to remove from your selection: ");
                int id; scanf("%d", &id); getchar();
                int found = 0;
                for (int i = 0; i < orders->count; i++) {
                    if (orders->items[i].guest_id == guest_id &&
                        orders->items[i].gift_id  == id) {
                        /* shift */
                        for (int j = i; j < orders->count - 1; j++)
                            orders->items[j] = orders->items[j+1];
                        orders->count--;
                        found = 1;
                        printf("  [OK]  Gift removed from your selection.\n");
                        break;
                    }
                }
                if (!found) printf("  [!]  Gift ID %d not found in your selection.\n", id);
                press_enter();
                break;
            }
            case 3: {
                float total = compute_user_total(orders, guest_id);
                if (total == 0.0f) {
                    printf("  [!]  You have not selected any gifts.\n");
                    press_enter(); break;
                }
                printf("\n  -- Order Summary for %s --\n", guest_name);
                print_separator();
                display_user_orders(orders, guest_id);
                printf("\n  TOTAL COST: %.2f XAF\n", total);
                printf("\n  Confirm your selection? (1=Yes / 0=No): ");
                int confirm; scanf("%d", &confirm); getchar();
                if (confirm == 1) {
                    save_gifts(list, orders);
                    printf("  [OK]  Your gift selection has been saved. Thank you!\n");
                    press_enter();
                    running = 0;
                } else {
                    printf("  Selection not confirmed. You may continue editing.\n");
                    press_enter();
                }
                break;
            }
            case 0:
                running = 0;
                break;
            default:
                printf("  [!]  Invalid option.\n");
                press_enter();
        }
    }
}

/* ===============================================
 *  ADMIN MENU
 * =============================================== */

void gift_admin_menu(GiftList *list, OrderList *orders) {
    int running = 1;
    while (running) {
        clear_screen();
        print_title("GIFT MANAGEMENT -- ADMIN");
        printf("\n  1. Load gift list from file  (%s)\n", GIFTS_CSV);
        printf("  2. Display gift catalogue\n");
        printf("  3. Display all reservations\n");
        printf("  4. Display reservations by popularity\n");
        printf("  5. Update gift catalogue (add / edit / remove)\n");
        printf("  6. Save catalogue & orders to file\n");
        printf("  7. Show total value of all reserved gifts\n");
        printf("  0. Back to main menu\n");
        printf("\n  Choice: ");

        int choice;
        if (scanf("%d", &choice) != 1) { getchar(); continue; }
        getchar();

        switch (choice) {
            case 1:
                load_gifts(list);
                load_orders(orders);
                printf("  [OK]  Loaded %d gift(s) and %d order(s).\n",
                       list->count, orders->count);
                press_enter();
                break;
            case 2:
                display_gifts(list);
                press_enter();
                break;
            case 3:
                display_all_orders(orders);
                press_enter();
                break;
            case 4:
                display_orders_by_frequency(orders, list);
                press_enter();
                break;
            case 5:
                update_gifts(list);
                break;
            case 6:
                save_gifts(list, orders);
                press_enter();
                break;
            case 7:
                printf("\n  Total value of all reserved gifts: %.2f XAF\n",
                       total_gift_value(orders));
                press_enter();
                break;
            case 0:
                running = 0;
                break;
            default:
                printf("  [!]  Invalid option.\n");
                press_enter();
        }
    }
}

/* ===============================================
 *  USER MENU
 * =============================================== */

void gift_user_menu(GiftList *list, OrderList *orders) {
    clear_screen();
    print_title("GIFT MANAGEMENT -- GUEST");

    if (list->count == 0) {
        printf("\n  [!]  No gift catalogue loaded yet.\n");
        printf("      Please ask the admin to load the gift list first.\n");
        press_enter();
        return;
    }

    /* Identify guest */
    printf("\n  Enter your Guest ID  : ");
    int guest_id; scanf("%d", &guest_id); getchar();

    printf("  Enter your full name : ");
    char guest_name[100];
    fgets(guest_name, sizeof(guest_name), stdin);
    strip_newline(guest_name);

    int running = 1;
    while (running) {
        clear_screen();
        printf("\n  -- Guest Gift Portal -- %s --\n", guest_name);
        print_separator();
        printf("  1. Browse catalogue & choose gifts\n");
        printf("  2. View my current selection & total\n");
        printf("  0. Back to main menu\n");
        printf("\n  Choice: ");

        int choice;
        if (scanf("%d", &choice) != 1) { getchar(); continue; }
        getchar();

        switch (choice) {
            case 1:
                choose_gifts(list, orders, guest_id, guest_name);
                break;
            case 2:
                display_user_orders(orders, guest_id);
                printf("  Total: %.2f XAF\n", compute_user_total(orders, guest_id));
                press_enter();
                break;
            case 0:
                running = 0;
                break;
            default:
                printf("  [!]  Invalid option.\n");
                press_enter();
        }
    }
}

/* ===============================================
 *  MAIN ENTRY POINT FOR GIFT MODULE
 * =============================================== */

void gift_main_menu(GiftList *list, OrderList *orders) {
    int running = 1;
    while (running) {
        clear_screen();
        print_title("WIGMS -- GIFT MANAGEMENT MODULE");
        printf("\n  1. Admin section  (password required)\n");
        printf("  2. Guest section\n");
        printf("  0. Back to home\n");
        printf("\n  Choice: ");

        int choice;
        if (scanf("%d", &choice) != 1) { getchar(); continue; }
        getchar();

        switch (choice) {
            case 1: {
                /* Password protection */
                printf("\n  Enter admin password: ");
                char pwd[64];
                fgets(pwd, sizeof(pwd), stdin);
                strip_newline(pwd);
                if (strcmp(pwd, ADMIN_PASSWORD) != 0) {
                    printf("  [!]  Incorrect password. Access denied.\n");
                    press_enter();
                    break;
                }
                gift_admin_menu(list, orders);
                break;
            }
            case 2:
                gift_user_menu(list, orders);
                break;
            case 0:
                running = 0;
                break;
            default:
                printf("  [!]  Invalid option.\n");
                press_enter();
        }
    }
}

/* ===============================================
 *  STANDALONE TEST MAIN
 *  (remove or guard with #ifdef when integrating)
 * =============================================== */
int main(void) {
    GiftList  gifts  = {0};
    OrderList orders = {0};

    /* Pre-load from CSV if they exist */
    load_gifts(&gifts);
    load_orders(&orders);

    gift_main_menu(&gifts, &orders);
    return 0;
}
