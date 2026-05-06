/*
 * gift.h — WIGMS
 * Gift Management Module Header
 *
 * Follows the structure convention of Person_.h
 */

#ifndef GIFT_H
#define GIFT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

/* ── Admin password ──────────────────────────── */
#define ADMIN_PASSWORD "pokemon"
#define GIFTS_CSV      "gifts.csv"
#define ORDERS_CSV     "gift_orders.csv"
#define MAX_GIFTS      100
#define MAX_ORDERS     500

/* ── Gift status ─────────────────────────────── */
typedef enum {
    AVAILABLE,
    RESERVED
} GiftStatus;

/* ── Core Gift structure ─────────────────────── */
typedef struct {
    int    gift_id;
    char   name[100];
    float  price;
    char   description[200];
    GiftStatus status;
} Gift;

/* ── Gift Order: links a Guest to a Gift ─────── */
typedef struct {
    int  order_id;
    int  guest_id;
    char guest_name[100];
    int  gift_id;
    char gift_name[100];
    float gift_price;
} GiftOrder;

/* ── Gift list (dynamic array) ───────────────── */
typedef struct {
    Gift  items[MAX_GIFTS];
    int   count;
} GiftList;

/* ── Order list (dynamic array) ─────────────── */
typedef struct {
    GiftOrder items[MAX_ORDERS];
    int        count;
} OrderList;

/* ── Module 4: Gift Management functions ─────── */

/* Admin section */
void   load_gifts(GiftList *list);
void   save_gifts(const GiftList *list, const OrderList *orders);
void   display_gifts(const GiftList *list);
void   display_all_orders(const OrderList *orders);
void   display_orders_by_frequency(const OrderList *orders, const GiftList *list);
float  total_gift_value(const OrderList *orders);
void   update_gifts(GiftList *list);
void   add_gift(GiftList *list);
void   remove_gift(GiftList *list, int gift_id);

/* User section */
void   choose_gifts(GiftList *list, OrderList *orders, int guest_id, const char *guest_name);
float  compute_user_total(const OrderList *orders, int guest_id);
void   display_user_orders(const OrderList *orders, int guest_id);

/* Utility */
void   strip_newline(char *s);
int    gift_exists(const GiftList *list, int gift_id);
int    already_ordered(const OrderList *orders, int guest_id, int gift_id);
int    next_order_id(const OrderList *orders);
int    next_gift_id(const GiftList *list);

/* Console UI helpers (gift.c only) */
void   gift_admin_menu(GiftList *list, OrderList *orders);
void   gift_user_menu(GiftList *list, OrderList *orders);
void   gift_main_menu(GiftList *list, OrderList *orders);

#endif /* GIFT_H */
