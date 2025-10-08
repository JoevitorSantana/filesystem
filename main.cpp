#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.c>

#define MAX_ENDERECOS_DIRETOS_INODE 5
#define MAX_ENDERECOS_INDIRETOS_SIMPLES_INODE 5
#define MAX_ENDERECOS_INDIRETOS_DUPLO_INODE 5
#define MAX_ENDERECOS_INDIRETOS_TRIPLO_INODE 5
#define MAX_ENDERECOS_BLOCOS_LISTA_LIVRE 10
#define MAX_NUM_BLOCOS_INDIRETO_SIMPLES 10
#define MAX_NUM_BLOCOS_INDIRETO_DUPLO 25
#define MAX_NUM_BLOCOS_INDIRETO_TRIPLO 125

struct inode
{
    int contadorHardLinks;
    int tamanho;
    char protecao[11];
    int enderecosDiretos[MAX_ENDERECOS_DIRETOS_INODE];
    int enderecoSimplesIndireto;
    int enderecosDuploIndireto;
    int enderecosTriploIndireto;
};
typedef struct inode Inode;

struct inodeindiretosimples
{
    int enderecos[MAX_ENDERECOS_INDIRETOS_SIMPLES_INODE];
    int quantidadeEnderecos;
};
typedef struct inodeindiretosimples INodeIndiretoSimples;

struct inodeindiretoduplo
{
    int enderecos[MAX_ENDERECOS_INDIRETOS_DUPLO_INODE];
    int quantidadeEnderecos;
};
typedef struct inodeindiretoduplo INodeIndiretoDuplo;

struct inodeindiretotriplo
{
    int enderecos[MAX_ENDERECOS_INDIRETOS_TRIPLO_INODE];
    int quantidadeEnderecos;
};
typedef struct inodeindiretotriplo INodeIndiretoTriplo;

struct entrada
{
    char nome[50];
    int bloco;
};
typedef struct entrada Entrada;

struct diretorio
{
    Entrada entradas[10];
    int quantidadeEntradas;
};
typedef struct diretorio Diretorio;

struct bloco
{
    char tipo;
    Inode inode;
    INodeIndiretoSimples inodeEnderecoSimples;
    INodeIndiretoDuplo inodeEnderecoDuplo;
    INodeIndiretoTriplo inodeEnderecoTriplo;
    Diretorio diretorio;
};
typedef struct bloco Bloco;

enum tipoBloco
{
    DIRETORIO = 'D',
    ARQUIVO = 'A',
    FREE = 'F',
    BAD = 'B',
    INODE = 'I'
};

enum Comandos
{
    LS = 'ls',
    MKDIR = 'mkdir',
    CD = 'cd',
    TOUCH = 'touch',
    VI = 'vi',
    RMDIR = 'rmdir',
    RM = 'rm',
    LINK = 'link',
    UNLINK = 'unlink',
    CHMOD = 'chmod',
    EXIT = 'exit'
};

#include "TADPilhaBlocos.h"
#include "TADListaEncadeadaBlocosLivres.h"

void listarDiretorio(Diretorio diretorio);
void inserirDiretorio(char *nome, ListaBlocosLivres *lista, Bloco *disco, int blocoDiretorioPai);
int abrirDiretorio(char *caminho, int blocoAtual, Bloco *disco);
void inserirArquivo(char *nome, int tamanhoEmBytes, ListaBlocosLivres *lista, Bloco *disco, int blocoDiretorio);
void removerArquivo(char *nome, int blocoAtual, Bloco *disco, ListaBlocosLivres *lista);

void inicializarInode(Inode &inode)
{
    inode.contadorHardLinks = 0;
    inode.tamanho = 0;
    inode.protecao[0] = '\0';
    for (int j = 0; j < MAX_ENDERECOS_DIRETOS_INODE; j++)
    {
        inode.enderecosDiretos[j] = -1;
    }
    inode.enderecosDiretos[0] = -1;
    inode.enderecoSimplesIndireto = -1;
    inode.enderecosDuploIndireto = -1;
    inode.enderecosTriploIndireto = -1;
}

void inicializarInodeSimples(INodeIndiretoSimples &inodeSimples)
{
    inodeSimples.quantidadeEnderecos = 0;
    for (int j = 0; j < MAX_ENDERECOS_INDIRETOS_SIMPLES_INODE; j++)
    {
        inodeSimples.enderecos[j] = -1;
    }
}

void inicializarInodeDuplo(INodeIndiretoDuplo &inodeDuplo)
{
    inodeDuplo.quantidadeEnderecos = 0;
    for (int j = 0; j < MAX_ENDERECOS_INDIRETOS_DUPLO_INODE; j++)
    {
        inodeDuplo.enderecos[j] = -1;
    }
}

void inicializarInodeTriplo(INodeIndiretoTriplo &inodeTriplo)
{
    inodeTriplo.quantidadeEnderecos = 0;
    for (int j = 0; j < MAX_ENDERECOS_INDIRETOS_TRIPLO_INODE; j++)
    {
        inodeTriplo.enderecos[j] = -1;
    }
}

void inicializarDiretorio(Diretorio &diretorio)
{
    diretorio.quantidadeEntradas = 0;
    for (int i = 0; i < 10; i++)
    {
        diretorio.entradas[i].nome[0] = '\0';
        diretorio.entradas[i].bloco = -1;
    }
}

void inicializarBloco(Bloco &bloco)
{
    bloco.tipo = FREE;
    inicializarDiretorio(bloco.diretorio);
    inicializarInode(bloco.inode);
    inicializarInodeSimples(bloco.inodeEnderecoSimples);
    inicializarInodeDuplo(bloco.inodeEnderecoDuplo);
    inicializarInodeTriplo(bloco.inodeEnderecoTriplo); 
}

void inicializarBlocos(Bloco *disco, int quantidadeBlocos)
{
    for (int i = 0; i < quantidadeBlocos; i++)
    {
        inicializarBloco(disco[i]);
    }
}

void inicializarDiretorio(Bloco *disco, int bloco, int blocoPai)
{
    disco[bloco].tipo = ARQUIVO;
    disco[bloco].diretorio.quantidadeEntradas = 2;
    strcpy(disco[bloco].diretorio.entradas[0].nome, ".");
    disco[bloco].diretorio.entradas[0].bloco = bloco;
    strcpy(disco[bloco].diretorio.entradas[1].nome, "..");
    disco[bloco].diretorio.entradas[1].bloco = blocoPai;
}

void iniciarTerminal(Bloco *disco, ListaBlocosLivres *listaBlocosLivres)
{
    char linha[256];
    char *comando;
    char *arg1, *arg2;
    char caminho[100] = "/";
    int blocoAtual = 0; // bloco do diretorio raiz

    do {
        textcolor(GREEN);
        printf("root@localhost");
        textcolor(WHITE);
        printf(":");
        textcolor(LIGHTBLUE);
        printf("%s", caminho);
        textcolor(WHITE);
        printf("$ ");

        if (fgets(linha, sizeof(linha), stdin) == NULL) break; 

        // Remove a quebra de linha ('\n') adicionada por fgets
        linha[strcspn(linha, "\n")] = '\0';

        // Separa a linha em tokens (comando e parâmetros)
        comando = strtok(linha, " ");

        // Se a linha estiver vazia, continua o loop
        if (comando == NULL) continue;

        if (strcmp(comando, "ls") == 0) {
            arg1 = strtok(NULL, " ");
            if (arg1 != NULL) {
                if (strcmp(arg1, "-l") == 0) {
                    listarDiretorio(disco[blocoAtual].diretorio);
                } else {
                    printf("Uso: ls [-l]\n");
                }
            } else
                listarDiretorio(disco[blocoAtual].diretorio);
        } else if(strcmp(comando, "mkdir") == 0) {
            arg1 = strtok(NULL, " ");
            if (arg1 != NULL)
                inserirDiretorio(arg1, listaBlocosLivres, disco, blocoAtual);
            else
                printf("Uso: mkdir <nome_diretorio>\n");
        } else if(strcmp(comando, "cd") == 0) {
            arg1 = strtok(NULL, " ");
            if (arg1 != NULL) {
                int blocoDiretorio = abrirDiretorio(arg1, blocoAtual, disco);
                blocoAtual = blocoDiretorio;
            } else {
                printf("Uso: cd <caminho_diretorio>\n");
            }
        } else if(strcmp(comando, "touch") == 0) {
            arg1 = strtok(NULL, " ");
            arg2 = strtok(NULL, " ");
            if (arg1 != NULL && arg2 != NULL) {
                int tamanho = atoi(arg2);
                inserirArquivo(arg1, tamanho, listaBlocosLivres, disco, blocoAtual);
            } else {
                printf("Uso: touch <nome_arquivo> <tamanho_em_bytes>\n");
            }
        } else if(strcmp(comando, "vi") == 0) {

        } else if(strcmp(comando, "rmdir") == 0) {

        } else if(strcmp(comando, "rm") == 0) {
            arg1 = strtok(NULL, " ");
            if (arg1 != NULL)
                removerArquivo(arg1, blocoAtual, disco, listaBlocosLivres);
            else
                printf("Uso: rm <nome_arquivo>\n");
        } else if(strcmp(comando, "link") == 0) {

        } else if(strcmp(comando, "unlink") == 0) {

        } else if(strcmp(comando, "chmod") == 0) {

        }
    } while (strcmp(comando, "exit") != 0);
}

void inicializarListaBlocosLivres(ListaBlocosLivres *lista, int quantidadeBlocos) {
    while (quantidadeBlocos > 0) {
        Pilha *pilha = CriarPilha();
        for (int i = 0; i < MAX_ENDERECOS_BLOCOS_LISTA_LIVRE && quantidadeBlocos > 0; i++) {
            Push(pilha, quantidadeBlocos - 1);
            quantidadeBlocos--;
        }
        InserirInicio(lista, pilha);
    }
}

int quantidadeBlocosLivres(ListaBlocosLivres *lista) {
    NoLista *Aux;
    Aux = lista->cabeca;
    int contador = 0;
    while(Aux != NULL) {
        contador = Count(Aux->blocos) + contador;
        Aux = Aux->prox;
    }
    return contador;
}

int alocarBloco(ListaBlocosLivres *lista)
{
    NoLista *Aux = lista->cabeca;
    if (Count(Aux->blocos) == 0 && Aux->prox != NULL) {
        RemoverInicio(lista);
        Aux = lista->cabeca;
    } else if (Count(Aux->blocos) == 0 && Aux->prox == NULL) {
        printf("Espaco insuficiente\n");
        return -1;
    }

    Pop(Aux->blocos);
}

void realocarBlocos(ListaBlocosLivres *lista, int bloco)
{
    // Se tiver espaço insere
    // Se não cria outro nó, insere na lista
    NoLista *Aux = lista->cabeca; 
    if (Count(Aux->blocos) < MAX_ENDERECOS_BLOCOS_LISTA_LIVRE) {
        Push(Aux->blocos, bloco);
    } else {
        Pilha *pilha = CriarPilha();
        Push(pilha, bloco);
        InserirInicio(lista, pilha);
    }
}

void alocarBlocosEInserirInode(ListaBlocosLivres *lista, int quantidade)
{
    // verificar se ha blocos suficientes
    if (quantidade > quantidadeBlocosLivres(lista)) {
        printf("Nao ha espaco suficiente\n");
        return;
    }
    // verificar se proximo no da lista nao eh nulo
    NoLista *Aux = lista->cabeca;
    while (quantidade > 0 && Aux != NULL) {
        // verificar se a pilha de blocos do no atual nao esta vazia
        while (quantidade > 0 && Count(Aux->blocos) > 0) {
            int bloco = Pop(Aux->blocos);
            printf("Bloco alocado: %d\n", bloco);
            quantidade--;
        }
        // se a pilha de blocos do no atual estiver vazia, ir para o proximo no
        if (Count(Aux->blocos) == 0){
            RemoverInicio(lista);
            Aux = lista->cabeca;
        }
    }
}

void inserirDiretorio(char *nome, ListaBlocosLivres *lista, Bloco *disco, int blocoDiretorioPai) {
    // validar quantidade de blocos livres
    if (quantidadeBlocosLivres(lista) < 2) {
        printf("Nao ha espaco suficiente\n");
        return;
    }
    // alocar um bloco para o inode
    int enderecoBlocoInode = alocarBloco(lista);
    disco[enderecoBlocoInode].tipo = DIRETORIO;
    // inserir inode do diretorio pai
    disco[blocoDiretorioPai].diretorio.entradas[disco[blocoDiretorioPai].diretorio.quantidadeEntradas].bloco = enderecoBlocoInode;
    strcpy(disco[blocoDiretorioPai].diretorio.entradas[disco[blocoDiretorioPai].diretorio.quantidadeEntradas++].nome, nome);

    int blocoArquivoDiretorio = alocarBloco(lista);
    // inicializar o diretorio (inserir . e ..)
    inicializarDiretorio(disco, blocoArquivoDiretorio, blocoDiretorioPai);

    disco[enderecoBlocoInode].inode.enderecosDiretos[0] = blocoArquivoDiretorio;
}

/*
 - TO DO
 - VERIFICAR BLOCOS INDIRETOS SIMPLES, DUPLOS E TRIPLOS
*/
void inserirArquivo(char *nome, int tamanhoEmBytes, ListaBlocosLivres *lista, Bloco *disco, int blocoDiretorio) {
    // Cada bloco tem 10 bytes
    int quantidadeBlocos = tamanhoEmBytes / 10;
    if (tamanhoEmBytes % 10 != 0)
        quantidadeBlocos++;

    if (quantidadeBlocos + 1 > quantidadeBlocosLivres(lista)) {
        printf("Nao ha espaco suficiente\n");
        return;
    }
    // inserir inode
    int enderecoBlocoInode = alocarBloco(lista);
    disco[enderecoBlocoInode].tipo = INODE;

    // inserir inode do diretorio pai
    disco[blocoDiretorio].diretorio.entradas[disco[blocoDiretorio].diretorio.quantidadeEntradas].bloco = enderecoBlocoInode;
    strcpy(disco[blocoDiretorio].diretorio.entradas[disco[blocoDiretorio].diretorio.quantidadeEntradas++].nome, nome);

    // alocar blocos do arquivo e inserir no inode
    NoLista *Aux = lista->cabeca;
    int quantidade = quantidadeBlocos;

    disco[enderecoBlocoInode].inode.tamanho = tamanhoEmBytes;
    disco[enderecoBlocoInode].inode.contadorHardLinks = 1;

    int quantidadeAlocada = 0;
    int bloco;

    INodeIndiretoSimples inodeSimples[MAX_ENDERECOS_INDIRETOS_SIMPLES_INODE];

    INodeIndiretoSimples inodeSimplesListaIndiretoTriplo[MAX_NUM_BLOCOS_INDIRETO_DUPLO];
    INodeIndiretoDuplo inodeDuploListaIndiretoTriplo[MAX_ENDERECOS_INDIRETOS_DUPLO_INODE];

    INodeIndiretoDuplo inodeDuplo;
    INodeIndiretoTriplo inodeTriplo;
    
    int qtdeInodeDuplo = 0, qtdeInodeSimples = 0;
    int qtdeInodeDuploLIT = 0, qtdeInodeSimplesLIT = 0;

    while (quantidade > 0 && Aux != NULL) {
        // verificar se a pilha de blocos do no atual nao esta vazia
        while (quantidade > 0 && Count(Aux->blocos) > 0) {
            if (quantidadeAlocada < MAX_ENDERECOS_DIRETOS_INODE) {
                bloco = alocarBloco(lista);
                disco[enderecoBlocoInode].inode.enderecosDiretos[quantidadeBlocos - quantidade] = bloco;
                disco[bloco].tipo = ARQUIVO;                
            } else if (quantidadeBlocos >= MAX_ENDERECOS_DIRETOS_INODE && quantidadeBlocos <= MAX_NUM_BLOCOS_INDIRETO_SIMPLES)
            {
                if (quantidadeAlocada == MAX_ENDERECOS_DIRETOS_INODE) {
                    // alocar um bloco para o inode de enderecos indiretos simples
                    int blocoInodeSimples = alocarBloco(lista);
                    disco[enderecoBlocoInode].inode.enderecoSimplesIndireto = blocoInodeSimples;
                    disco[blocoInodeSimples].tipo = INODE;
                }
                
                int posBlocoInodeIndiretoSimples = disco[enderecoBlocoInode].inode.enderecoSimplesIndireto;
                bloco = Pop(Aux->blocos);

                // inserir o bloco na lista de enderecos do inode de enderecos indiretos simples
                disco[posBlocoInodeIndiretoSimples].inodeEnderecoSimples.enderecos[disco[posBlocoInodeIndiretoSimples].inodeEnderecoSimples.quantidadeEnderecos++] = bloco;
                disco[bloco].tipo = ARQUIVO;
            } else if (quantidadeBlocos > MAX_NUM_BLOCOS_INDIRETO_SIMPLES && quantidadeBlocos <= MAX_NUM_BLOCOS_INDIRETO_DUPLO)
            {
                if (quantidadeAlocada == MAX_ENDERECOS_DIRETOS_INODE) {
                    // inicializar os inodos indiretos simples
                    inicializarInodeDuplo(inodeDuplo);

                    for (int i = 0; i < MAX_ENDERECOS_INDIRETOS_DUPLO_INODE; i++)
                        inicializarInodeSimples(inodeSimples[i]);
                }
                
                //  verificar a quantidade de inodes simples ja alocados
                if (inodeSimples[qtdeInodeSimples].quantidadeEnderecos == MAX_ENDERECOS_INDIRETOS_SIMPLES_INODE)
                    qtdeInodeSimples++;

                bloco = alocarBloco(lista);
                inodeSimples[qtdeInodeSimples].enderecos[inodeSimples[qtdeInodeSimples].quantidadeEnderecos++] = bloco;
                disco[bloco].tipo = ARQUIVO;

                if (quantidadeAlocada == quantidadeBlocos-1) {
                    // alocar um bloco para o inode de enderecos indiretos duplos
                    for (int i = 0; i <= qtdeInodeSimples; i++)
                    {
                        bloco = alocarBloco(lista);
                        disco[bloco].inodeEnderecoSimples = inodeSimples[i];
                        disco[bloco].tipo = INODE;

                        inodeDuplo.enderecos[inodeDuplo.quantidadeEnderecos++] = bloco;
                    }

                    int blocoInodeDuplo = alocarBloco(lista);
                    disco[enderecoBlocoInode].inode.enderecosDuploIndireto = blocoInodeDuplo;
                    disco[blocoInodeDuplo].tipo = INODE;
                    disco[blocoInodeDuplo].inodeEnderecoDuplo = inodeDuplo;
                }
            } else if (quantidadeBlocos > MAX_NUM_BLOCOS_INDIRETO_DUPLO && quantidadeBlocos <= MAX_NUM_BLOCOS_INDIRETO_TRIPLO)
            {
                if (quantidadeAlocada == MAX_ENDERECOS_DIRETOS_INODE) {
                    inicializarInodeTriplo(inodeTriplo);

                    // inializar os 5 inodes duplos
                    for (int i = 0; i < MAX_ENDERECOS_INDIRETOS_DUPLO_INODE;i++)
                    {
                        inicializarInodeDuplo(inodeDuploListaIndiretoTriplo[i]);
                    }
                    
                    // inicializar os 25 inodes simples
                    for (int i = 0; i < MAX_NUM_BLOCOS_INDIRETO_DUPLO; i++)
                    {
                        inicializarInodeSimples(inodeSimplesListaIndiretoTriplo[i]);
                    }
                }

                if (inodeSimplesListaIndiretoTriplo[qtdeInodeSimplesLIT].quantidadeEnderecos == MAX_ENDERECOS_INDIRETOS_DUPLO_INODE)
                    qtdeInodeSimplesLIT++;
                
                bloco = alocarBloco(lista);
                inodeSimplesListaIndiretoTriplo[qtdeInodeSimplesLIT].enderecos[inodeSimplesListaIndiretoTriplo[qtdeInodeSimplesLIT].quantidadeEnderecos++] = bloco;
                disco[bloco].tipo = ARQUIVO;

                if (quantidadeAlocada == quantidadeBlocos - 1)
                {
                    // percorrer os inodes simples qtdeInodeSimplesLIT
                    // para cada um alocar um bloco para o inode duplo 
                    for (int i = 0; i <= qtdeInodeSimplesLIT; i++)
                    {
                        bloco = alocarBloco(lista);
                        disco[bloco].inodeEnderecoSimples = inodeSimplesListaIndiretoTriplo[i];
                        disco[bloco].tipo = INODE;

                        // adicionar ao nó pai
                        inodeDuploListaIndiretoTriplo[qtdeInodeDuploLIT].enderecos[inodeDuploListaIndiretoTriplo[qtdeInodeDuploLIT].quantidadeEnderecos++] = bloco;

                        if (inodeDuploListaIndiretoTriplo[qtdeInodeDuploLIT].quantidadeEnderecos == MAX_ENDERECOS_INDIRETOS_DUPLO_INODE)
                            qtdeInodeDuploLIT++;
                    }

                    // percorrer os nós duplos alocar blocos e adicionar no noh triplo
                    for (int i = 0; i <= qtdeInodeDuploLIT; i++)
                    {
                        bloco = alocarBloco(lista);
                        disco[bloco].inodeEnderecoDuplo = inodeDuploListaIndiretoTriplo[i];
                        disco[bloco].tipo = INODE;

                        inodeTriplo.enderecos[inodeTriplo.quantidadeEnderecos++] = bloco;
                    }

                    bloco = alocarBloco(lista);
                    disco[bloco].inodeEnderecoTriplo = inodeTriplo;
                    disco[bloco].tipo = INODE;

                    // adicionar ao inode principal
                    disco[enderecoBlocoInode].inode.enderecosTriploIndireto = bloco;
                }
            }
            
            quantidadeAlocada++;
            quantidade--;
        }
        // se a pilha de blocos do no atual estiver vazia, ir para o proximo no
        if (quantidade > 0 && Count(Aux->blocos) == 0){
            RemoverInicio(lista);
            Aux = lista->cabeca;
        }
    }
}

void removerArquivo(char *nome, int blocoAtual, Bloco *disco, ListaBlocosLivres *lista) {
    // buscar o arquivo na entrada de diretorio
    int i = 0;
    int blocoInodeArquivo = -1;

    for (i = 2; i < disco[blocoAtual].diretorio.quantidadeEntradas; i++) {
        if (strcmp(disco[blocoAtual].diretorio.entradas[i].nome, nome) == 0) {
            blocoInodeArquivo = disco[blocoAtual].diretorio.entradas[i].bloco;
            break;
        }
    }
    // ir no inode 
    if (blocoInodeArquivo == -1) {
        printf("Arquivo nao encontrado\n");
        return;
    }
    // liberar os blocos do arquivo
    for (int j = 0; j < MAX_ENDERECOS_DIRETOS_INODE; j++) {
        if (disco[blocoInodeArquivo].inode.enderecosDiretos[j] != -1) {
            realocarBlocos(lista, disco[blocoInodeArquivo].inode.enderecosDiretos[j]);
            inicializarBloco(disco[disco[blocoInodeArquivo].inode.enderecosDiretos[j]]);
            // disco[disco[blocoInodeArquivo].inode.enderecosDiretos[j]].tipo = FREE;
            disco[blocoInodeArquivo].inode.enderecosDiretos[j] = -1;
        }
    }

    // liberar blocos inodes duplos
    if (disco[blocoInodeArquivo].inode.enderecoSimplesIndireto != -1)
    {
        // percorrer os enderecos do bloco indireto simples realoando os blocs
        int enderecoBlocoIndiretoSimples = disco[blocoInodeArquivo].inode.enderecoSimplesIndireto;

        for (int i = 0; i < disco[enderecoBlocoIndiretoSimples].inodeEnderecoSimples.quantidadeEnderecos; i++)
        {
            int blocoRealocar = disco[enderecoBlocoIndiretoSimples].inodeEnderecoSimples.enderecos[i];
            realocarBlocos(lista, blocoRealocar);
            inicializarBloco(disco[blocoRealocar]);

        }

        realocarBlocos(lista, enderecoBlocoIndiretoSimples);
        inicializarBloco(disco[enderecoBlocoIndiretoSimples]);
        disco[blocoInodeArquivo].inode.enderecoSimplesIndireto = -1;
    }

    // remover de inode duplo indireto
    if (disco[blocoInodeArquivo].inode.enderecosDuploIndireto != -1)
    {
        int enderecoBlocoIndiretoDuplo = disco[blocoInodeArquivo].inode.enderecosDuploIndireto;

        for(int i = 0; i < disco[enderecoBlocoIndiretoDuplo].inodeEnderecoDuplo.quantidadeEnderecos; i++)
        {
            int enderecoBlocoIndiretoSimples = disco[enderecoBlocoIndiretoDuplo].inodeEnderecoDuplo.enderecos[i];

            for (int j = 0; j < disco[enderecoBlocoIndiretoSimples].inodeEnderecoSimples.quantidadeEnderecos; j++)
            {
                int blocoRealocar = disco[enderecoBlocoIndiretoSimples].inodeEnderecoSimples.enderecos[j];
                realocarBlocos(lista, blocoRealocar);
                inicializarBloco(disco[blocoRealocar]);
            }
            
            realocarBlocos(lista, enderecoBlocoIndiretoSimples);
            inicializarBloco(disco[enderecoBlocoIndiretoSimples]);
        }

        realocarBlocos(lista, enderecoBlocoIndiretoDuplo);
        inicializarBloco(disco[enderecoBlocoIndiretoDuplo]);
        disco[blocoInodeArquivo].inode.enderecosDuploIndireto = -1;
    }

    // remover de inode triplo indireto
    if (disco[blocoInodeArquivo].inode.enderecosTriploIndireto != -1)
    {
        int enderecoBlocoIndiretoTriplo = disco[blocoInodeArquivo].inode.enderecosTriploIndireto;

        for (int i = 0; i < disco[enderecoBlocoIndiretoTriplo].inodeEnderecoTriplo.quantidadeEnderecos; i++)
        {
            int enderecoBlocoIndiretoDuplo = disco[enderecoBlocoIndiretoTriplo].inodeEnderecoTriplo.enderecos[i];

            for(int i = 0; i < disco[enderecoBlocoIndiretoDuplo].inodeEnderecoDuplo.quantidadeEnderecos; i++)
            {
                int enderecoBlocoIndiretoSimples = disco[enderecoBlocoIndiretoDuplo].inodeEnderecoDuplo.enderecos[i];

                for (int j = 0; j < disco[enderecoBlocoIndiretoSimples].inodeEnderecoSimples.quantidadeEnderecos; j++)
                {
                    int blocoRealocar = disco[enderecoBlocoIndiretoSimples].inodeEnderecoSimples.enderecos[j];
                    realocarBlocos(lista, blocoRealocar);
                    inicializarBloco(disco[blocoRealocar]);
                }
                
                realocarBlocos(lista, enderecoBlocoIndiretoSimples);
                inicializarBloco(disco[enderecoBlocoIndiretoSimples]);
            }

            realocarBlocos(lista, enderecoBlocoIndiretoDuplo);
            inicializarBloco(disco[enderecoBlocoIndiretoDuplo]);
            disco[blocoInodeArquivo].inode.enderecosDuploIndireto = -1;
        }

        realocarBlocos(lista, enderecoBlocoIndiretoTriplo);
        inicializarBloco(disco[enderecoBlocoIndiretoTriplo]);
        disco[blocoInodeArquivo].inode.enderecosTriploIndireto = -1;
    }
    // remover o inode do arquivo
    realocarBlocos(lista, blocoInodeArquivo);
    inicializarBloco(disco[blocoInodeArquivo]);

    // remover a entrada do diretorio    
    for (int j = i; j < disco[blocoAtual].diretorio.quantidadeEntradas - 1; j++) {
        disco[blocoAtual].diretorio.entradas[j] = disco[blocoAtual].diretorio.entradas[j + 1];
    }
    disco[blocoAtual].diretorio.quantidadeEntradas--;
}

void listarDiretorio(Diretorio diretorio) {
    printf("\n");
    for (int i = 2; i < diretorio.quantidadeEntradas; i++) {
        printf("%s\t", diretorio.entradas[i].nome);
    }
    printf("\n");
}

int split_path(char *path, char **parts, int max_parts) {
    if (path == NULL || parts == NULL || max_parts <= 0) {
        return -1; // Erro: entradas inválidas
    }

    // A primeira barra pode ser ignorada, a menos que o caminho seja apenas "/"
    if (strcmp(path, "/") == 0) {
        parts[0] = "/";
        return 1;
    }

    int count = 0;
    char *token = strtok(path, "/");

    while (token != NULL && count < max_parts) {
        parts[count] = token;
        count++;
        token = strtok(NULL, "/");
    }

    return count;
}

int abrirDiretorio(char *caminho, int blocoAtual, Bloco *disco) {
    // buscar no diretorio atual o nome do diretorio a ser aberto
    int blocoDiretorio = -1;

    char *path = new char[strlen(caminho) + 1];;
    strcpy(path, caminho);
    char *parts[10];
    int num_parts;

    num_parts = split_path(path, parts, 10);
    
    int i = 0, j;
    while (i < num_parts && blocoAtual != -1) {
        // buscar o nome do diretorio na lista de entradas do diretorio atual
        // se não encontrar, retornar diretório inválido
        int achou = 0;
        j = 0;
        while (j < disco[blocoAtual].diretorio.quantidadeEntradas && !achou) {
            if (strcmp(disco[blocoAtual].diretorio.entradas[j].nome, parts[i]) == 0) {
                int blocoInodeDiretorio = disco[blocoAtual].diretorio.entradas[j].bloco;
                if (strcmp(parts[i], "..") != 0 && strcmp(parts[i], ".") != 0) {
                    blocoDiretorio = disco[blocoInodeDiretorio].inode.enderecosDiretos[0];
                    blocoAtual = blocoDiretorio;
                } else {
                    blocoDiretorio = disco[blocoAtual].diretorio.entradas[j].bloco;
                    blocoAtual = blocoDiretorio;
                }
                achou = 1;
            }
            j++;
        }

        if (!achou) {
            printf("Diretorio invalido\n");
            return -1;
        }

        i++;
    }

    return blocoDiretorio;
}

void pwd(int blocoAtual, int blocoEntradaDiretorio, Bloco *disco) {
    // imprimir o caminho absoluto do diretorio atual
    // Ir buscando o bloco pai ate chegar no bloco raiz
    if (blocoEntradaDiretorio != -1) {
        for (int i = 2; i < disco[blocoAtual].diretorio.quantidadeEntradas; i++) {
            if (disco[blocoAtual].diretorio.entradas[i].bloco == blocoEntradaDiretorio)
                printf("%s/", disco[blocoAtual].diretorio.entradas[i].nome);
        }
    }
    
    if (blocoAtual == 0) {
        return;
    }

    for (int i = 0; i < disco[blocoAtual].diretorio.quantidadeEntradas; i++) {
        if (strcmp(disco[blocoAtual].diretorio.entradas[i].nome, "..") == 0) {
            int blocoPai = disco[blocoAtual].diretorio.entradas[i].bloco;
            pwd(blocoPai, blocoAtual, disco);
            break;
        }
    }
} 

int main(void)
{
    // INFORMAR QUANTIDADE DE BLOCOS
    // int quantidadeBlocos = 20;  
    int quantidadeBlocos = 1000;  

    //printf("Informe a quantidade de blocos: ");
    //scanf("%d", &quantidadeBlocos);

    Bloco disco[quantidadeBlocos];

    // INICIALIZAR BLOCOS COMO LIVRES
    inicializarBlocos(disco, quantidadeBlocos);

    // Inicializalizar a lista de blocos livres
    ListaBlocosLivres *listaBlocosLivres = CriarListaBlocosLivres();
    inicializarListaBlocosLivres(listaBlocosLivres, quantidadeBlocos);

    // INICIALIZAR O DIRETORIO RAIZ
    int blocoRaiz = alocarBloco(listaBlocosLivres);
    inicializarDiretorio(disco, blocoRaiz, blocoRaiz);


    // ##### INSERCOES DE TESTE #####
    // Criação de um arquivo
    // Alocrar um bloco para o inode
    // inserir o numero do bloco no inode e o nome do arquivo na entrada do diretório pai
    // Aloca os blocos do arquivo no inode
    // inserirArquivo("arquivo1.txt", 1000, listaBlocosLivres, disco, blocoRaiz);
    // inserirArquivo("arquivo2.txt", 25, listaBlocosLivres, disco, blocoRaiz);
    // inserirArquivo("arquivo3.txt", 25, listaBlocosLivres, disco, blocoRaiz);
    // inserirArquivo("arquivo4.txt", 25, listaBlocosLivres, disco, blocoRaiz);
    // inserirArquivo("arquivo5.txt", 25, listaBlocosLivres, disco, blocoRaiz);
    //inserirDiretorio("diretorio1", listaBlocosLivres, disco, blocoRaiz);
    // abrir diretorio
    //int blocoAtual = abrirDiretorio("diretorio1", blocoRaiz, disco);
    //removerArquivo("arquivo1.txt", blocoRaiz, disco, listaBlocosLivres);
    //inserirArquivo("arquivo_diretorio1.txt", 150, listaBlocosLivres, disco, blocoAtual);

    // ###### FIM INSERCOES DE TESTE #######
    iniciarTerminal(disco, listaBlocosLivres);

    // inicializarDiretorioRaiz(disco, blocoRaiz);
    // textcolor(GREEN);
    // printf("root@localhost");
    // textcolor(WHITE);
    // printf(":");
    // textcolor(LIGHTBLUE);
    // printf("%s", "/");
    // textcolor(WHITE);
    // printf("$ ");
    // listarDiretorio(disco[blocoAtual].diretorio);

    // percorrer blocos e imprimir seus tipos
    for (int i = 0; i < quantidadeBlocos; i++)
    {
        Bloco b = disco[i];
        printf("Bloco %d: Tipo %c\n", i, disco[i].tipo);
    }

    return 0;
}