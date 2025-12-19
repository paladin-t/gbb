#ifndef __LINKED_LIST_H__
#define __LINKED_LIST_H__

#include <types.h>

// #define STRICT_LINKED_LIST

#define LL_PUSH_HEAD(HEAD, ITEM) \
    (ITEM)->next = (HEAD); \
    (HEAD) = (ITEM)
#define LL_REMOVE_ITEM(HEAD, ITEM, PREV) \
    if (PREV) { \
        (PREV)->next = (ITEM)->next; \
    } else { \
        (HEAD) = (ITEM)->next; \
    }
#define LL_REMOVE_HEAD(HEAD) \
    if (HEAD) { \
        (HEAD) = (HEAD)->next; \
    }

#define DL_PUSH_HEAD(HEAD, ITEM) \
    (ITEM)->prev = NULL; \
    (ITEM)->next = (HEAD); \
    if (HEAD) { \
        (HEAD)->prev = (ITEM); \
    } \
    (HEAD) = (ITEM)
#define DL_PUSH_TAIL(TAIL, ITEM) \
    (ITEM)->prev = (TAIL); \
    (ITEM)->next = NULL; \
    if (TAIL) { \
        (TAIL)->next = (ITEM); \
    } \
    (TAIL) = (ITEM)
#if defined STRICT_LINKED_LIST
#define DL_REMOVE_ITEM(HEAD, ITEM) \
    if (HEAD) { \
        if ((ITEM)->next && (ITEM)->prev) { \
            (ITEM)->prev->next = (ITEM)->next; \
            (ITEM)->next->prev = (ITEM)->prev; \
        } else if ((ITEM)->next) { \
            (ITEM)->next->prev = NULL; \
            (HEAD) = (ITEM)->next; \
        } else if ((ITEM)->prev) { \
            (ITEM)->prev->next = NULL; \
        } else { \
            (HEAD) = NULL; \
        } \
        (ITEM)->next = (ITEM)->prev = NULL; \
    }
#else /* STRICT_LINKED_LIST */
#define DL_REMOVE_ITEM(HEAD, ITEM) \
    if ((ITEM)->next && (ITEM)->prev) { \
        (ITEM)->prev->next = (ITEM)->next; \
        (ITEM)->next->prev = (ITEM)->prev; \
    } else if ((ITEM)->next) { \
        (ITEM)->next->prev = NULL; \
        (HEAD) = (ITEM)->next; \
    } else if ((ITEM)->prev) { \
        (ITEM)->prev->next = NULL; \
    } else { \
        (HEAD) = NULL; \
    }
#endif /* STRICT_LINKED_LIST */
#define DL_CONTAINS(HEAD_MUT, ITEM, FOUND) \
    (FOUND) = FALSE; \
    while (HEAD_MUT) { \
        if ((HEAD_MUT) == (ITEM)) { \
            (FOUND) = TRUE; \
            break; \
        } \
        (HEAD_MUT) = (HEAD_MUT)->next; \
    }

#endif /* __LINKED_LIST_H__ */
