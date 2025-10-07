struct No {
    int bloco;
    No *prox;
};

struct Pilha {
    No *topo;
};

Pilha *CriarPilha (void) {
    Pilha *p = new Pilha;
    p->topo = NULL;
    return p;
}

void Push(Pilha *pilha, int bloco) {
    No *novo = new No;
    novo->bloco = bloco;
    novo->prox = pilha->topo;
    pilha->topo = novo;
}

void LiberarPilha(Pilha *pilha) {
    No *Aux = pilha->topo;
    while (Aux != NULL) {
        No *Aux2 = Aux->prox;
        delete(Aux);
        Aux = Aux2;
    }
    delete(pilha);
}

int Pop(Pilha *pilha) {
    No *Aux;
    int bloco;

    if (pilha->topo != NULL) {
        Aux = pilha->topo;
        bloco = Aux->bloco;
        pilha->topo = Aux->prox;
        delete Aux;
    }
    return bloco;
}

int Count(Pilha *pilha) {
    No *Aux;
    Aux = pilha->topo;
    int total = 0;
    while (Aux != NULL) {
        total++;
        Aux = Aux->prox;
    }
    return total;
}

void Imprimir(Pilha *pilha) {
    No *Aux;
    for (Aux = pilha->topo; Aux != NULL; Aux = Aux->prox)
        printf("%d ", Aux->bloco);
    printf("\n");
}