
#ifndef ORDER_LINK_H
#define ORDER_LINK_H

#ifdef __cplusplus
extern "C" {
#endif
typedef struct link {
    int  elem;
    struct link* next;
}Link;
Link* initLink();
void pushElem(Link* p, int elem);
int popElem(Link* p);
int getElemSize(Link* p);
void Link_free(Link* p);
void Link_display(Link* p);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*ORDER_LINK_H*/
