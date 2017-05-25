/*Compilerbau, Sommersemester 2017, Gruppe 20: Wilhelm Bomke, Jakub Jalowiec & David Bachorska  */
#include <stdio.h>
#include <stdlib.h>
#include "stack.h"

int stackInit(intstack_t* self) {         
    if(self == NULL) {                                             
    fprintf(stderr, "Error: NULL-Pointer, param self @ stackInit.\n");
    return 1;
    }
    self->top = NULL;  /*initialisiert den Zeiger auf das oberste Stackelement mit NULL*/
    return 0;
}

void stackRelease(intstack_t * self) {
    if(self->top == NULL) {
        fprintf(stderr, "Error: Stack is empty. param self @ stackRelease \n");
        exit(1);
    }
    stackelement_t * temp = NULL;  /*"Hilfszeiger" zum freigeben des dynamisch angeforderten Speichers*/
    while(self->top) {
        temp = self->top->ptr;
        free(self);
        self->top = temp;   
    }
}


void stackPush(intstack_t* self, int i) {
    stackelement_t * newElement = malloc(sizeof(stackelement_t));  /*Speicher für ein Stackelement anfordern*/
    if(newElement == NULL) {
        fprintf(stderr, "Error: Coudln't allocate memory @ stackPush. \n");
        exit(1);
    }
    newElement->value = i;
    newElement->ptr = self->top;
    self->top = newElement;
        
    }


int stackTop(const intstack_t* self) {
    return self->top->value;  /*Gibt das oberste Element im Stack zurück*/
}

int stackPop(intstack_t* self) {
    if(self->top == NULL) {  /*Wenn der Stack kein Element mehr enthält kann kein Element mehr entfernt werden*/
        fprintf(stderr, "Error: Stack is empty. param self @ stackPop \n");
        exit(1);
    }
    int temp = self->top->value;
    stackelement_t* tempp = self->top->ptr;
    free(self->top);
    self->top = tempp;
    return temp;
}

int stackIsEmpty(const intstack_t* self) {
    if(self->top == NULL) {  /*Wenn das oberste Stackelement == NULL dann ist der Stack leer*/
        return 1;
    }
    return 0;
}
void stackPrint(const intstack_t* self) {
    if(self->top == NULL) {  /*Überprüfen ob der Stack leer ist*/
        fprintf(stderr, "Error: Stack is empty. param self @ stackPrint \n");
        exit(1);
    }
    stackelement_t * temp;
    temp = self->top;
    while(temp) {
        printf("%i \n" ,temp->value);
        temp = temp->ptr;
    }
}
