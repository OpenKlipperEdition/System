/**
 * @file     list.h
 * @brief    定义了链表结构体以及操作方式
 * @author     fuyu.peng (fuyu.peng@ingenic.com)
 * @version      1.0
 * @date     2023.10.27
 */
#ifndef __LIST_H__
#define __LIST_H__

#include <stddef.h>

/**
 * @brief  链表结构体定义 
 */
struct list_head
{
    struct list_head *next;     /*!< next指针，指向下一个链表 */
    struct list_head *prev;     /*!< prev指针，指向上一个链表 */
};

/**
 * @brief    链表头初始化函数
 * @param[in] list 链表头地址
 */
static inline void INIT_LIST_HEAD(struct list_head *list)                                                                                                              
{
    list->next = list;
    list->prev = list;
}

/**
 * @brief    向链表头添加数据
 * @param[in] new  要添加的数据项地址
 * @param[in] head 链表头地址
 */
static inline void list_add_head(struct list_head *new, struct list_head *head)
{
    struct list_head *temp;
    temp = head->prev;

    new->next = head;
    new->prev = head->prev;
    temp->next = new;
    head->prev = new;
    
}

/**
 * @brief    向链表头添加数据
 * @param[in] new  要添加的数据项地址
 * @param[in] head 链表头地址
 */
static inline void list_add_tail(struct list_head *new, struct list_head *head)
{
    struct list_head *temp;
    temp = head->next;

    new->next = head->next;
    new->prev = head;
    temp->prev = new;
    head->next = new;
}

/**
 * @brief    删除数据项
 * @param[in] entry　要删除的数据项地址
 */
static inline void list_del(struct list_head *entry)
{
    entry->next->prev = entry->prev;
    entry->prev->next = entry->next;

//    entry->next = NULL;
//    entry->prev = NULL;
    entry->next = entry;
    entry->prev = entry;
}

/**
 * @brief   判断链表是否为空
 * @param[in] head 头节点地址
 * @return int         链表为空返回１，不空返回０
 */
static inline int list_empty(const struct list_head *head)
{
        return head->next == head;
}   

#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})


/**
 * @brief       取数据项
 */
#define list_entry(ptr, type, member) \
        container_of(ptr, type, member)

/**
 * @brief    取第一项数据项
 */
#define list_first_entry(ptr, type, member) \
        list_entry((ptr)->next, type, member)

/**
 * @brief    取下一项数据项
 */
#define list_next_entry(pos, member) \
        list_entry((pos)->member.next, typeof(*(pos)), member)

/**
 * @brief    遍历链表数据项
 */
#define list_for_each_entry(pos, head, member)                          \
        for (pos = list_first_entry(head, typeof(*pos), member);        \
             &pos->member != (head);                                    \
             pos = list_next_entry(pos, member))
#endif
