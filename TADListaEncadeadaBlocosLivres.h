struct NoLista {
    Pilha *blocos;
    NoLista *prox;
};

struct ListaBlocosLivres {
    NoLista *cabeca;
};

ListaBlocosLivres *CriarListaBlocosLivres (void) {
    ListaBlocosLivres *p = new ListaBlocosLivres;
    p->cabeca = NULL;
    return p;
}

void InserirInicio(ListaBlocosLivres *lista, Pilha *blocos) {
    NoLista *novo = new NoLista;
    novo->blocos = blocos;
    novo->prox = lista->cabeca;
    lista->cabeca = novo;
}

void LiberarPilha(ListaBlocosLivres *pilha) {
    NoLista *Aux;
    while (Aux != NULL) {
        NoLista *Aux2 = Aux->prox;
        delete(Aux2);
        Aux = Aux2;
    }
    delete(pilha);
}

Pilha* RemoverInicio(ListaBlocosLivres *lista) {
    NoLista *Aux;
    Pilha *blocos;

    if (lista->cabeca != NULL) {
        Aux = lista->cabeca;
        blocos = Aux->blocos;
        lista->cabeca = Aux->prox;
        delete Aux;
    }
    return blocos;
}

// void Imprimir(ListaBlocosLivres *lista) {
//     NoLista *Aux;
//     for (Aux = lista->cabeca; Aux != NULL; Aux = Aux->prox)
//         printf("%d ", Aux->blocos);
//     printf("\n");
// }