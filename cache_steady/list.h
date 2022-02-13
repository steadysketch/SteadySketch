#ifndef _LIST_H_
#define _LIST_H_

#include <string>
#include "utils.h"
using namespace std;

class list_node
{
public:
    string *value;
    list_node *pre;
    list_node *next;
    ~list_node() { delete value; }
};

class list
{
protected:
    list_node *head;
    list_node *tail;
    int len;
    int limit;

public:
    list(int _limit) : limit(_limit)
    {
        head = new list_node;
        tail = new list_node;
        head->value = tail->value = NULL;
        head->pre = tail->next = NULL;
        head->next = tail;
        tail->pre = head;
        len = 0;
    }
    list_node *insert(const string &x)
    {
        if (len == limit)
            return NULL;
        list_node *tmp = new list_node;
        tmp->value = new string(x);
        tmp->pre = tail->pre;
        tmp->next = tail;
        tmp->pre->next = tmp;
        tmp->next->pre = tmp;
        ++len;
        return tmp;
    }
    bool is_full() const { return len == limit; }
    string *get_first_item() const { return head->next->value; }
    void del_first_item()
    {
        list_node *tmp = head->next;
        tmp->next->pre = head;
        head->next = tmp->next;
        delete tmp;
        --len;
    }
    void erase(list_node *x)
    {
        x->pre->next = x->next;
        x->next->pre = x->pre;
        delete x;
        --len;
    }
    void adjust(list_node *x)
    {
        x->pre->next = x->next;
        x->next->pre = x->pre;
        x->pre = tail->pre;
        x->next = tail;
        x->pre->next = x;
        x->next->pre = x;
    }
    list_node *find(const string &x)
    {
        for (list_node *it = head->next; it != tail; it = it->next)
            if (*(it->value) == x)
                return it;
        return NULL;
    }
    ~list()
    {
        for (list_node *i = head->next, *j = i->next; i != tail; i = j, j = j->next)
            delete i;
        delete head;
        delete tail;
    }
};

#endif