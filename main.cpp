#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <conio.c>
#include <sys/types.h>
#include <sys/wait.h> // No windiws deu b.o tem que ver
#include <unistd.h>
#include <time.h>
#include <cstdio>
#include <ctype.h>
#include "conio.h"
#include <time.h>

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
    char tipo;
    int contadorHardLinks;
    int tamanho;
    char protecao[11];
    char data[11];
    char hora[6];
    char usuario[20];
    char grupo[20];

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
    int blocosLivres[10];
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

enum Comando
{
    LS,
    MKDIR,
    CD,
    TOUCH,
    VI,
    RMDIR,
    RM,
    LINK,
    UNLINK,
    CHMOD,
    EXIT
};

#include "TADPilhaBlocos.h"
#include "TADListaEncadeadaBlocosLivres.h"

void listarDiretorio(Diretorio diretorio);
void inserirDiretorio(char *nome, ListaBlocosLivres *lista, Bloco *disco, int blocoDiretorioPai);
int abrirDiretorio(char *caminho, int blocoAtual, Bloco *disco);
void inserirArquivo(char *nome, int tamanhoEmBytes, ListaBlocosLivres *lista, Bloco *disco, int blocoDiretorio);
void removerArquivo(char *nome, int blocoAtual, Bloco *disco, ListaBlocosLivres *lista);
int localizarInodePorNome(char *nome, int blocoAtual, Bloco *disco);
void alterarPermissao(char *operacao, char *escopo, char *modos, char *nome, int blocoAtual, Bloco *disco);
void listarDiretorioDetalhado(Diretorio diretorio, Bloco *disco);
int temPermissao(Inode inode, char tipo);
int alocarBloco(ListaBlocosLivres *lista);
int quantidadeBlocosLivres(ListaBlocosLivres *lista);
void criarLinkSimbolico(char *origem, char *destino, int blocoAtual, ListaBlocosLivres *lista, Bloco *disco);
void removerLinkSimbolico(char *nome, int blocoAtual, Bloco *disco, ListaBlocosLivres *lista);

void inicializarInode(Inode *inode)
{
    inode->tipo = '-';
    inode->protecao[0] = '\0';
    strcpy(inode->protecao, "rwxr-xr-x");

    inode->data[0] = '\0';
    inode->hora[0] = '\0';
    strcpy(inode->usuario, "root");
    strcpy(inode->grupo, "root");

    inode->tamanho = 0;
    inode->contadorHardLinks = 0;

    for (int j = 0; j < MAX_ENDERECOS_DIRETOS_INODE; j++)
        inode->enderecosDiretos[j] = -1;

    inode->enderecoSimplesIndireto = -1;
    inode->enderecosDuploIndireto = -1;
    inode->enderecosTriploIndireto = -1;
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
    inicializarInode(&bloco.inode);
    inicializarInodeSimples(bloco.inodeEnderecoSimples);
    inicializarInodeDuplo(bloco.inodeEnderecoDuplo);
    inicializarInodeTriplo(bloco.inodeEnderecoTriplo);
}

void inicializarBlocos(Bloco *disco, int quantidadeBlocos)
{
    for (int i = 0; i < quantidadeBlocos; i++)
    {
        inicializarBloco(disco[i]);
        // disco[i].tipo = FREE;
        // inicializarDiretorio(disco[i].diretorio);
        // inicializarInode(&disco[i].inode);
    }
}

void inicializarDiretorio(Bloco *disco, int bloco, int blocoPai)
{
    disco[bloco].tipo = DIRETORIO;
    disco[bloco].diretorio.quantidadeEntradas = 2;
    strcpy(disco[bloco].diretorio.entradas[0].nome, ".");
    disco[bloco].diretorio.entradas[0].bloco = bloco;
    strcpy(disco[bloco].diretorio.entradas[1].nome, "..");
    disco[bloco].diretorio.entradas[1].bloco = blocoPai;

    for (int i = 2; i < 10; i++)
    {
        disco[bloco].diretorio.entradas[i].nome[0] = '\0';
        disco[bloco].diretorio.entradas[i].bloco = -1;
    }
}

void inicializarListaBlocosLivres(ListaBlocosLivres *lista, int quantidadeBlocos)
{
    while (quantidadeBlocos > 0)
    {
        Pilha *pilha = CriarPilha();
        for (int i = 0; i < MAX_ENDERECOS_BLOCOS_LISTA_LIVRE && quantidadeBlocos > 0; i++)
        {
            Push(pilha, quantidadeBlocos - 1);
            quantidadeBlocos--;
        }
        InserirInicio(lista, pilha);
    }
}

void gravarListaBlocosLivresDisco(ListaBlocosLivres *lista, Bloco *disco)
{
    // gravar do disco lista de blocos livres
    int blocosLista = quantidadeBlocosLivres(lista);

    while (blocosLista > 0)
    {
        int bloco = alocarBloco(lista);
        disco[bloco].tipo = ARQUIVO;
        blocosLista -= 10;
    }
}

int quantidadeBlocosLivres(ListaBlocosLivres *lista)
{
    NoLista *Aux;
    Aux = lista->cabeca;
    int contador = 0;
    while (Aux != NULL)
    {
        contador = Count(Aux->blocos) + contador;
        Aux = Aux->prox;
    }
    return contador;
}

int alocarBloco(ListaBlocosLivres *lista)
{
    NoLista *Aux = lista->cabeca;
    if (Aux == NULL)
    {
        printf("Espaco insuficiente\n");
        return -1;
    }

    if (Count(Aux->blocos) == 0 && Aux->prox != NULL)
    {
        RemoverInicio(lista);
        Aux = lista->cabeca;
        if (Aux == NULL)
        { // segurança extra
            printf("Espaco insuficiente\n");
            return -1;
        }
    }
    else if (Count(Aux->blocos) == 0 && Aux->prox == NULL)
    {
        printf("Espaco insuficiente\n");
        return -1;
    }

    int bloco = Pop(Aux->blocos); // Pop deve retornar o índice do bloco
    return bloco;
}

void realocarBlocos(ListaBlocosLivres *lista, int bloco)
{
    // Se tiver espaço insere
    // Se não cria outro nó, insere na lista
    NoLista *Aux = lista->cabeca;
    if (Count(Aux->blocos) < MAX_ENDERECOS_BLOCOS_LISTA_LIVRE)
    {
        Push(Aux->blocos, bloco);
    }
    else
    {
        Pilha *pilha = CriarPilha();
        Push(pilha, bloco);
        InserirInicio(lista, pilha);
    }
}

void alocarBlocosEInserirInode(ListaBlocosLivres *lista, int quantidade)
{
    // verificar se ha blocos suficientes
    if (quantidade > quantidadeBlocosLivres(lista))
    {
        printf("Nao ha espaco suficiente\n");
        return;
    }
    // verificar se proximo no da lista nao eh nulo
    NoLista *Aux = lista->cabeca;
    while (quantidade > 0 && Aux != NULL)
    {
        // verificar se a pilha de blocos do no atual nao esta vazia
        while (quantidade > 0 && Count(Aux->blocos) > 0)
        {
            int bloco = Pop(Aux->blocos);
            printf("Bloco alocado: %d\n", bloco);
            quantidade--;
        }
        // se a pilha de blocos do no atual estiver vazia, ir para o proximo no
        if (Count(Aux->blocos) == 0)
        {
            RemoverInicio(lista);
            Aux = lista->cabeca;
        }
    }
}

void inserirDiretorio(char *nome, ListaBlocosLivres *lista, Bloco *disco, int blocoDiretorioPai)
{
    // Verificar blocos livres
    if (quantidadeBlocosLivres(lista) < 2)
    {
        printf("Nao ha espaco suficiente\n");
        return;
    }

    // Verificar limite de entradas do diretório pai
    if (disco[blocoDiretorioPai].diretorio.quantidadeEntradas >= 10)
    {
        printf("Erro: diretorio cheio!\n");
        return;
    }

    // Alocar bloco para o inode do novo diretório
    int enderecoBlocoInode = alocarBloco(lista);
    disco[enderecoBlocoInode].tipo = INODE;

    // Inicializar inode
    Inode *inode = &disco[enderecoBlocoInode].inode;
    inode->tamanho = 0;
    inode->contadorHardLinks = 1;
    inode->tipo = 'd';

    for (int i = 0; i < 10; i++)
        inode->enderecosDiretos[i] = -1;

    // Adicionar data e hora
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    strftime(inode->data, sizeof(inode->data), "%d/%m/%Y", tm_info);
    strftime(inode->hora, sizeof(inode->hora), "%H:%M", tm_info);

    // Inserir no diretório pai
    int idx = disco[blocoDiretorioPai].diretorio.quantidadeEntradas;
    strcpy(disco[blocoDiretorioPai].diretorio.entradas[idx].nome, nome);
    disco[blocoDiretorioPai].diretorio.entradas[idx].bloco = enderecoBlocoInode;
    disco[blocoDiretorioPai].diretorio.quantidadeEntradas++;

    // Alocar bloco de dados do novo diretório
    int blocoArquivoDiretorio = alocarBloco(lista);

    // Inicializar novo diretório (. e ..)
    inicializarDiretorio(disco, blocoArquivoDiretorio, blocoDiretorioPai);

    // Apontar no inode do diretório novo
    inode->enderecosDiretos[0] = blocoArquivoDiretorio;
}
/*
 - TO DO
 - VERIFICAR BLOCOS INDIRETOS SIMPLES, DUPLOS E TRIPLOS
*/
void inserirArquivo(char *nome, int tamanhoEmBytes, ListaBlocosLivres *lista, Bloco *disco, int blocoDiretorio)
{
    // Cada bloco tem 10 bytes
    int quantidadeBlocos = tamanhoEmBytes / 10;
    if (tamanhoEmBytes % 10 != 0)
        quantidadeBlocos++;

    if (quantidadeBlocos + 1 > quantidadeBlocosLivres(lista))
    {
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

    Inode *inode = &disco[enderecoBlocoInode].inode;
    inode->tamanho = tamanhoEmBytes;
    inode->contadorHardLinks = 1;
    inode->tipo = '-';

    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    strftime(inode->data, sizeof(inode->data), "%d/%m/%Y", tm_info);
    strftime(inode->hora, sizeof(inode->hora), "%H:%M", tm_info);

    int quantidadeAlocada = 0;
    int bloco;

    INodeIndiretoSimples inodeSimples[MAX_ENDERECOS_INDIRETOS_SIMPLES_INODE];

    INodeIndiretoSimples inodeSimplesListaIndiretoTriplo[MAX_NUM_BLOCOS_INDIRETO_DUPLO];
    INodeIndiretoDuplo inodeDuploListaIndiretoTriplo[MAX_ENDERECOS_INDIRETOS_DUPLO_INODE];

    INodeIndiretoDuplo inodeDuplo;
    INodeIndiretoTriplo inodeTriplo;

    int qtdeInodeDuplo = 0, qtdeInodeSimples = 0;
    int qtdeInodeDuploLIT = 0, qtdeInodeSimplesLIT = 0;

    while (quantidade > 0 && Aux != NULL)
    {
        // verificar se a pilha de blocos do no atual nao esta vazia
        while (quantidade > 0 && Count(Aux->blocos) > 0)
        {
            if (quantidadeAlocada < MAX_ENDERECOS_DIRETOS_INODE)
            {
                bloco = alocarBloco(lista);
                disco[enderecoBlocoInode].inode.enderecosDiretos[quantidadeBlocos - quantidade] = bloco;
                disco[bloco].tipo = ARQUIVO;
            }
            else if (quantidadeBlocos >= MAX_ENDERECOS_DIRETOS_INODE && quantidadeBlocos <= MAX_NUM_BLOCOS_INDIRETO_SIMPLES)
            {
                if (quantidadeAlocada == MAX_ENDERECOS_DIRETOS_INODE)
                {
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
            }
            else if (quantidadeBlocos > MAX_NUM_BLOCOS_INDIRETO_SIMPLES && quantidadeBlocos <= MAX_NUM_BLOCOS_INDIRETO_DUPLO)
            {
                if (quantidadeAlocada == MAX_ENDERECOS_DIRETOS_INODE)
                {
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

                if (quantidadeAlocada == quantidadeBlocos - 1)
                {
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
            }
            else if (quantidadeBlocos > MAX_NUM_BLOCOS_INDIRETO_DUPLO && quantidadeBlocos <= MAX_NUM_BLOCOS_INDIRETO_TRIPLO)
            {
                if (quantidadeAlocada == MAX_ENDERECOS_DIRETOS_INODE)
                {
                    inicializarInodeTriplo(inodeTriplo);

                    // inializar os 5 inodes duplos
                    for (int i = 0; i < MAX_ENDERECOS_INDIRETOS_DUPLO_INODE; i++)
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
        if (quantidade > 0 && Count(Aux->blocos) == 0)
        {
            RemoverInicio(lista);
            Aux = lista->cabeca;
        }
    }
}

void removerArquivo(char *nome, int blocoAtual, Bloco *disco, ListaBlocosLivres *lista)
{
    // buscar o arquivo na entrada de diretorio
    int i = 0;
    int blocoInodeArquivo = -1;

    for (i = 2; i < disco[blocoAtual].diretorio.quantidadeEntradas; i++)
    {
        if (strcmp(disco[blocoAtual].diretorio.entradas[i].nome, nome) == 0)
        {
            blocoInodeArquivo = disco[blocoAtual].diretorio.entradas[i].bloco;
            break;
        }
    }
    // ir no inode
    if (blocoInodeArquivo == -1)
    {
        printf("Arquivo nao encontrado\n");
        return;
    }
    // liberar os blocos do arquivo
    for (int j = 0; j < MAX_ENDERECOS_DIRETOS_INODE; j++)
    {
        if (disco[blocoInodeArquivo].inode.enderecosDiretos[j] != -1)
        {
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

        for (int i = 0; i < disco[enderecoBlocoIndiretoDuplo].inodeEnderecoDuplo.quantidadeEnderecos; i++)
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

            for (int i = 0; i < disco[enderecoBlocoIndiretoDuplo].inodeEnderecoDuplo.quantidadeEnderecos; i++)
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
    for (int j = i; j < disco[blocoAtual].diretorio.quantidadeEntradas - 1; j++)
    {
        disco[blocoAtual].diretorio.entradas[j] = disco[blocoAtual].diretorio.entradas[j + 1];
    }
    disco[blocoAtual].diretorio.quantidadeEntradas--;
}

void removerDiretorio(char *nome, int blocoAtual, Bloco *disco, ListaBlocosLivres *lista)
{
    // buscar o diretorio
    int i = 0;
    int blocoInodeArquivo = -1;

    for (i = 2; i < disco[blocoAtual].diretorio.quantidadeEntradas; i++)
    {
        if (strcmp(disco[blocoAtual].diretorio.entradas[i].nome, nome) == 0)
        {
            blocoInodeArquivo = disco[blocoAtual].diretorio.entradas[i].bloco;
            break;
        }
    }
    // ir no inode
    if (blocoInodeArquivo == -1)
    {
        printf("Arquivo nao encontrado\n");
        return;
    }

    if (disco[blocoInodeArquivo].inode.tipo != 'd')
    {
        printf("\nInforme um diretorio valido!\n");
        return;
    }

    // verificar se tem arquivo
    int blocoDiretorio = disco[blocoInodeArquivo].inode.enderecosDiretos[0];

    if (blocoDiretorio != -1 && disco[blocoDiretorio].diretorio.quantidadeEntradas > 2)
    {
        printf("\nEste diretorio possui arquivos!\n");
        return;
    }

    // liberar os blocos do arquivo
    for (int j = 0; j < MAX_ENDERECOS_DIRETOS_INODE; j++)
    {
        if (disco[blocoInodeArquivo].inode.enderecosDiretos[j] != -1)
        {
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

        for (int i = 0; i < disco[enderecoBlocoIndiretoDuplo].inodeEnderecoDuplo.quantidadeEnderecos; i++)
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

            for (int i = 0; i < disco[enderecoBlocoIndiretoDuplo].inodeEnderecoDuplo.quantidadeEnderecos; i++)
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
    for (int j = i; j < disco[blocoAtual].diretorio.quantidadeEntradas - 1; j++)
    {
        disco[blocoAtual].diretorio.entradas[j] = disco[blocoAtual].diretorio.entradas[j + 1];
    }
    disco[blocoAtual].diretorio.quantidadeEntradas--;
}

void listarDiretorio(Diretorio diretorio)
{
    printf("\n");
    for (int i = 2; i < diretorio.quantidadeEntradas; i++)
    {
        printf("%s\t", diretorio.entradas[i].nome);
    }
    printf("\n");
}

int split_path(char *path, char **parts, int max_parts)
{
    if (path == NULL || parts == NULL || max_parts <= 0)
    {
        return -1; // Erro: entradas inválidas
    }

    // A primeira barra pode ser ignorada, a menos que o caminho seja apenas "/"
    if (strcmp(path, "/") == 0)
    {
        parts[0] = "/";
        return 1;
    }

    int count = 0;
    char *token = strtok(path, "/");

    while (token != NULL && count < max_parts)
    {
        parts[count] = token;
        count++;
        token = strtok(NULL, "/");
    }

    return count;
}

int abrirDiretorio(char *caminho, int blocoAtual, Bloco *disco)
{
    // buscar no diretorio atual o nome do diretorio a ser aberto
    int blocoDiretorio = -1;

    char *path = new char[strlen(caminho) + 1];
    strcpy(path, caminho);
    char *parts[10];
    int num_parts;

    num_parts = split_path(path, parts, 10);

    int i = 0, j;
    while (i < num_parts && blocoAtual != -1)
    {
        // buscar o nome do diretorio na lista de entradas do diretorio atual
        // se não encontrar, retornar diretório inválido
        int achou = 0;
        j = 0;
        while (j < disco[blocoAtual].diretorio.quantidadeEntradas && !achou)
        {
            if (strcmp(disco[blocoAtual].diretorio.entradas[j].nome, parts[i]) == 0)
            {
                int blocoInodeDiretorio = disco[blocoAtual].diretorio.entradas[j].bloco;
                if (strcmp(parts[i], "..") != 0 && strcmp(parts[i], ".") != 0)
                {
                    blocoDiretorio = disco[blocoInodeDiretorio].inode.enderecosDiretos[0];
                    blocoAtual = blocoDiretorio;
                }
                else
                {
                    blocoDiretorio = disco[blocoAtual].diretorio.entradas[j].bloco;
                    blocoAtual = blocoDiretorio;
                }
                achou = 1;
            }
            j++;
        }

        if (!achou)
        {
            printf("Diretorio invalido\n");
            return -1;
        }

        i++;
    }

    return blocoDiretorio;
}

void pwd(int blocoAtual, int blocoEntradaDiretorio, Bloco *disco)
{
    // imprimir o caminho absoluto do diretorio atual
    // Ir buscando o bloco pai ate chegar no bloco raiz
    if (blocoEntradaDiretorio != -1)
    {
        for (int i = 2; i < disco[blocoAtual].diretorio.quantidadeEntradas; i++)
        {
            if (disco[blocoAtual].diretorio.entradas[i].bloco == blocoEntradaDiretorio)
                printf("%s/", disco[blocoAtual].diretorio.entradas[i].nome);
        }
    }

    if (blocoAtual == 0)
    {
        return;
    }

    for (int i = 0; i < disco[blocoAtual].diretorio.quantidadeEntradas; i++)
    {
        if (strcmp(disco[blocoAtual].diretorio.entradas[i].nome, "..") == 0)
        {
            int blocoPai = disco[blocoAtual].diretorio.entradas[i].bloco;
            pwd(blocoPai, blocoAtual, disco);
            break;
        }
    }
}

int localizarInodePorNome(char *nome, int blocoAtual, Bloco *disco)
{
    for (int i = 0; i < disco[blocoAtual].diretorio.quantidadeEntradas; i++)
    {
        if (strcmp(disco[blocoAtual].diretorio.entradas[i].nome, nome) == 0)
        {
            return disco[blocoAtual].diretorio.entradas[i].bloco;
        }
    }

    return -1;
}

void listarDiretorioDetalhado(Diretorio diretorio, Bloco *disco)
{
    printf("%-5s %-11s %-6s %-20s %-8s %-8s %10s %-12s %s\n",
           "Tipo ", "Permissões ", "Links ", "       Nome       ", "Usuário ", " Grupo ",
           "  Tamanho", "    Data    ", "Hora");

    printf("%-5s %-11s %-6s %-20s %-8s %-8s %10s %-12s %s\n",
           "-----", "-----------", "------", "--------------------", "--------", "--------",
           "----------", "------------", "-----");

    for (int i = 2; i < diretorio.quantidadeEntradas; i++)
    {
        int blocoInode = diretorio.entradas[i].bloco;

        if (disco[blocoInode].tipo == INODE)
        {
            Inode inode = disco[blocoInode].inode;
            char tipo;
            char perm[10];

            printf("%-5c %-11s %-6d %-20s %-8s %-8s %10d %-12s %s\n",
                   inode.tipo,
                   inode.protecao,
                   inode.contadorHardLinks,
                   diretorio.entradas[i].nome,
                   inode.usuario[0] ? inode.usuario : "root",
                   inode.grupo[0] ? inode.grupo : "root",
                   inode.tamanho,
                   inode.data[0] ? inode.data : "00/00/0000",
                   inode.hora[0] ? inode.hora : "00:00");
        }
        else
        {
            printf("-    ----------  -----  --------  --------  -------  ----------  -----  %s (bloco não é inode)\n",
                   diretorio.entradas[i].nome);
        }
    }
    printf("\n");
}

void alterarPermissao(char *operacao, char *escopo, char *modos, char *nome, int blocoAtual, Bloco *disco)
{
    if (!operacao || !escopo || !modos || !nome)
    {
        printf("Uso: chmod (+|-) (u|g|o) (rwx) <arquivo>\n");
        return;
    }

    int blocoInode = localizarInodePorNome(nome, blocoAtual, disco);
    if (blocoInode == -1)
    {
        printf("Arquivo ou diretório não encontrado: %s\n", nome);
        return;
    }

    Inode *inode = &disco[blocoInode].inode;

    if (strlen(inode->protecao) < 9)
        strcpy(inode->protecao, "rwxr-xr-x");

    int inicio = 0;
    if (strcmp(escopo, "u") == 0)
        inicio = 0;
    else if (strcmp(escopo, "g") == 0)
        inicio = 3;
    else if (strcmp(escopo, "o") == 0)
        inicio = 6;
    else
    {
        printf("Escopo inválido. Use u (usuário), g (grupo) ou o (outros).\n");
        return;
    }

    int adicionar = (strcmp(operacao, "+") == 0);
    int remover = (strcmp(operacao, "-") == 0);

    if (!adicionar && !remover)
    {
        printf("Operação inválida. Use + ou -.\n");
        return;
    }

    for (int i = 0; modos[i] != '\0'; i++)
    {
        char m = modos[i];
        int pos = -1;

        if (m == 'r')
            pos = 0;
        else if (m == 'w')
            pos = 1;
        else if (m == 'x')
            pos = 2;
        else
            continue;

        if (adicionar)
            inode->protecao[inicio + pos] = m;
        else if (remover)
            inode->protecao[inicio + pos] = '-';
    }

    printf("Permissões de '%s' atualizadas: %s\n", nome, inode->protecao);
}

int temPermissao(Inode inode, char tipo)
{
    if (strlen(inode.protecao) < 3)
        return 1;

    switch (tipo)
    {
    case 'r':
        return inode.protecao[0] == 'r';
    case 'w':
        return inode.protecao[1] == 'w';
    case 'x':
        return inode.protecao[2] == 'x';
    default:
        return 0;
    }
}

// BAAAADDDDD

void marcarBlocoBad(int numeroBloco, Bloco *disco, ListaBlocosLivres *lista, int quantidadeBlocos)
{
    // Verifica se o bloco é válido
    if (numeroBloco < 0 || numeroBloco >= quantidadeBlocos)
    {
        printf("Número de bloco inválido.\n");
        return;
    }

    // Verifica se já está marcado como bad
    if (disco[numeroBloco].tipo == BAD)
    {
        printf("Bloco %d já está marcado como defeituoso.\n", numeroBloco);
        return;
    }

    // Muda o tipo para BAD
    disco[numeroBloco].tipo = BAD;

    // Remover o bloco da lista de blocos livres (se estiver lá)
    NoLista *aux = lista->cabeca;
    while (aux != NULL)
    {
        Pilha *pilha = aux->blocos;
        Pilha *novaPilha = CriarPilha(); // nova pilha sem o bloco defeituoso

        while (pilha->topo != NULL)
        {
            int bloco = Pop(pilha);
            if (bloco != numeroBloco)
            {
                Push(novaPilha, bloco);
            }
        }

        // Substitui a pilha antiga pela nova
        aux->blocos = novaPilha;
        aux = aux->prox;
    }

    printf("Bloco %d foi marcado como defeituoso (BAD).\n", numeroBloco);
}

// RELATORIOOO BAD

void relatorioArquivosCorrompidos(Bloco *disco, int quantidadeBlocos)
{
    printf("\n=== Relatorio de Arquivos Corrompidos ===\n");

    int totalArquivos = 0;
    int arquivosInteiros = 0;
    int arquivosCorrompidos = 0;

    // Percorrer todos os blocos do disco
    for (int i = 0; i < quantidadeBlocos; i++)
    {
        if (disco[i].tipo == INODE)
        {
            int corrompido = 0;

            // Verifica os blocos diretos
            for (int j = 0; j < MAX_ENDERECOS_DIRETOS_INODE; j++)
            {
                int bloco = disco[i].inode.enderecosDiretos[j];
                if (bloco != -1)
                {
                    if (disco[bloco].tipo == BAD)
                    {
                        corrompido = 1;
                        break;
                    }
                }
            }

            totalArquivos++;
            // Buscar o nome do arquivo nos diretórios para exibir
            char nomeArquivo[50] = "arquivo_desconhecido";
            // Percorre diretórios procurando esse inode
            for (int d = 0; d < quantidadeBlocos; d++)
            {
                if (disco[d].tipo == DIRETORIO)
                {
                    for (int e = 0; e < disco[d].diretorio.quantidadeEntradas; e++)
                    {
                        if (disco[d].diretorio.entradas[e].bloco == i)
                        {
                            strcpy(nomeArquivo, disco[d].diretorio.entradas[e].nome);
                        }
                    }
                }
            }

            if (corrompido)
            {
                printf(" Arquivo corrompido: %s (inode %d)\n", nomeArquivo, i);
                arquivosCorrompidos++;
            }
            else
            {
                printf("Arquivo integro: %s (inode %d)\n", nomeArquivo, i);
                arquivosInteiros++;
            }
        }
    }

    printf("\nResumo: %d arquivos totais | %d integros | %d corrompidos\n",
           totalArquivos, arquivosInteiros, arquivosCorrompidos);
    printf("===========================================\n\n");
}

// DFFFFF

void mostrarEspacoDisco(Bloco *disco, int quantidadeBlocos, int tamanhoBloco)
{
    int livres = 0;
    int ocupados = 0;
    int defeituosos = 0;

    for (int i = 0; i < quantidadeBlocos; i++)
    {
        if (disco[i].tipo == FREE)
        {
            livres++;
        }
        else if (disco[i].tipo == BAD)
        {
            defeituosos++;
        }
        else
        {
            // Arquivo, Inode ou Diretório contam como ocupados
            ocupados++;
        }
    }

    int totalBytes = quantidadeBlocos * tamanhoBloco;
    int livresBytes = livres * tamanhoBloco;
    int ocupadosBytes = ocupados * tamanhoBloco;
    int defeituososBytes = defeituosos * tamanhoBloco;
    float percentualUso = (ocupados * 100.0) / quantidadeBlocos;

    printf("\n=== Relatorio de Espaco em Disco (df) ===\n");
    printf("Tamanho total do disco: %d bytes\n", totalBytes);
    printf("Espaco livre: %d bytes (%d blocos)\n", livresBytes, livres);
    printf("Espaco ocupado: %d bytes (%d blocos)\n", ocupadosBytes, ocupados);
    // printf("Blocos defeituosos: %d (%d bytes)\n", defeituosos, defeituososBytes);
    // printf("Uso do disco: %.2f%%\n", percentualUso);
    // printf("=========================================\n\n");
}

//// VIIIIII

void visualizarArquivo(char *nome, int blocoAtual, Bloco *disco, int quantidadeBlocos)
{
    // Procurar o arquivo no diretório atual
    int i;
    int blocoInode = -1;

    for (i = 2; i < disco[blocoAtual].diretorio.quantidadeEntradas; i++)
    {
        if (strcmp(disco[blocoAtual].diretorio.entradas[i].nome, nome) == 0)
        {
            blocoInode = disco[blocoAtual].diretorio.entradas[i].bloco;
            break;
        }
    }

    if (blocoInode == -1)
    {
        printf("Arquivo '%s' não encontrado.\n", nome);
        return;
    }

    // Verificar se o inode realmente é de arquivo
    if (disco[blocoInode].tipo != INODE)
    {
        printf("'%s' nao e um arquivo regular.\n", nome);
        return;
    }

    // Verificar se algum bloco de dados do arquivo está defeituoso
    int corrompido = 0;
    for (int j = 0; j < MAX_ENDERECOS_DIRETOS_INODE; j++)
    {
        int bloco = disco[blocoInode].inode.enderecosDiretos[j];
        if (bloco != -1 && disco[bloco].tipo == BAD)
        {
            corrompido = 1;
            break;
        }
    }

    if (corrompido)
        printf("Arquivo '%s' corrompido. Nao e possivel abri\n", nome);
    else
        printf("Arquivo '%s' visualizado com sucesso\n", nome);
}

/// RELATORIOOO 1
int blocosOcupadosArquivo(char *nome, int blocoDiretorio, Bloco *disco)
{
    // Procurar o arquivo no diretório atual
    int blocoInodeArquivo = -1;
    for (int i = 2; i < disco[blocoDiretorio].diretorio.quantidadeEntradas; i++)
    {
        if (strcmp(disco[blocoDiretorio].diretorio.entradas[i].nome, nome) == 0)
        {
            blocoInodeArquivo = disco[blocoDiretorio].diretorio.entradas[i].bloco;
            break;
        }
    }

    if (blocoInodeArquivo == -1)
    {
        printf("Arquivo '%s' não encontrado.\n", nome);
        return -1;
    }

    int blocosOcupados = 0;

    // Contar blocos diretos
    for (int j = 0; j < MAX_ENDERECOS_DIRETOS_INODE; j++)
    {
        if (disco[blocoInodeArquivo].inode.enderecosDiretos[j] != -1)
        {
            blocosOcupados++;
        }
    }

    // (Se quiser, depois dá pra incluir indiretos simples, duplos e triplos aqui)

    printf("O arquivo '%s' ocupa %d blocos de dados (%d bytes).\n",
           nome, blocosOcupados, blocosOcupados * 10);

    return blocosOcupados;
}

// Relatorio 5
void imprimirEstadoBlocos(Bloco *disco, int quantidadeBlocos)
{
    printf("\n=== ESTADO ATUAL DOS BLOCOS ===\n");
    printf("Bloco\tTipo\n");
    printf("--------------------------\n");

    for (int i = 0; i < quantidadeBlocos; i++)
    {
        char *tipo;

        switch (disco[i].tipo)
        {
        case 'D':
            tipo = "DIRETÓRIO";
            break;
        case 'A':
            tipo = "ARQUIVO";
            break;
        case 'I':
            tipo = "INODE";
            break;
        case 'F':
            tipo = "LIVRE";
            break;
        case 'B':
            tipo = "BAD";
            break;
        default:
            tipo = "DESCONHECIDO";
            break;
        }

        printf("%d\t%s\n", i, tipo);
    }
    printf("--------------------------\n");
}

// Relatorio 2

int maiorArquivoPossivel(ListaBlocosLivres *lista)
{
    int livres = quantidadeBlocosLivres(lista);

    if (livres <= 1)
    {
        printf("Espaço insuficiente para criar um novo arquivo.\n");
        return 0;
    }

    // 1 bloco será usado pelo inode
    int blocosDisponiveis = livres - 1;

    printf("O maior arquivo que ainda pode ser criado ocupa %d blocos (%d bytes).\n",
           blocosDisponiveis, blocosDisponiveis * 10);

    return blocosDisponiveis;
}

void criarLinkSimbolico(char *origem, char *destino, int blocoAtual, ListaBlocosLivres *lista, Bloco *disco)
{
    int blocoInodeOrigem;
    int blocoInodeLink;
    int i, j;
    int tamanho;
    int blocosNecessarios;
    int posicao;
    int blocoConteudo;
    char caminhoCompleto[256];

    blocoInodeOrigem = localizarInodePorNome(origem, blocoAtual, disco);
    if (blocoInodeOrigem == -1)
    {
        printf("Erro: origem '%s' não encontrada\n", origem);
        return;
    }

    if (localizarInodePorNome(destino, blocoAtual, disco) != -1)
    {
        printf("Erro: '%s' já existe\n", destino);
        return;
    }

    // Simula a obtenção do caminho completo exigido pelo enunciado
    strcpy(caminhoCompleto, origem);
    tamanho = strlen(caminhoCompleto);
    blocosNecessarios = (tamanho / 10) + 1;

    if (quantidadeBlocosLivres(lista) < blocosNecessarios + 1)
    {
        printf("Erro: espaço insuficiente\n");
        return;
    }

    blocoInodeLink = alocarBloco(lista);

    if (blocoInodeLink == -1)
    {
        printf("Erro: Falha na alocação do INODE para o link simbólico.\n");
        return;
    }

    disco[blocoInodeLink].tipo = INODE;

    Inode *inode = &disco[blocoInodeLink].inode;
    inicializarInode(inode);

    // Copia as permissões do INODE de origem (o alvo)
    Inode *inodeOrigem = &disco[blocoInodeOrigem].inode;
    strcpy(inode->protecao, inodeOrigem->protecao);

    inode->tipo = 'l';
    inode->tamanho = tamanho;
    inode->contadorHardLinks = 1;

    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    strftime(inode->data, sizeof(inode->data), "%d/%m/%Y", tm_info);
    strftime(inode->hora, sizeof(inode->hora), "%H:%M", tm_info);

    posicao = 0;
    i = 0;
    while (i < blocosNecessarios && i < MAX_ENDERECOS_DIRETOS_INODE)
    {
        blocoConteudo = alocarBloco(lista);

        if (blocoConteudo == -1)
        {
            printf("Erro: Falha inesperada na alocação de bloco de conteúdo.\n");
            // Em um FS real, seria necessário liberar o blocoInodeLink aqui.
            return;
        }

        disco[blocoConteudo].tipo = ARQUIVO;
        inode->enderecosDiretos[i] = blocoConteudo;

        j = 0;
        while (j < 10 && caminhoCompleto[posicao] != '\0')
        {
            disco[blocoConteudo].diretorio.entradas[0].nome[j] = caminhoCompleto[posicao];
            posicao++;
            j++;
        }
        disco[blocoConteudo].diretorio.entradas[0].nome[j] = '\0';

        i++;
    }

    i = disco[blocoAtual].diretorio.quantidadeEntradas;
    strcpy(disco[blocoAtual].diretorio.entradas[i].nome, destino);
    disco[blocoAtual].diretorio.entradas[i].bloco = blocoInodeLink;
    disco[blocoAtual].diretorio.quantidadeEntradas++;

    printf("Link simbólico '%s' -> '%s' criado com sucesso\n", destino, origem);
}

void removerLinkSimbolico(char *nome, int blocoAtual, Bloco *disco, ListaBlocosLivres *lista)
{
    int blocoInode;
    int i, j;

    blocoInode = localizarInodePorNome(nome, blocoAtual, disco);
    if (blocoInode == -1)
    {
        printf("Erro: link simbólico '%s' não encontrado\n", nome);
        return;
    }

    if (disco[blocoInode].inode.tipo != 'l')
    {
        printf("Erro: '%s' não é um link simbólico\n", nome);
        return;
    }

    i = 0;
    while (i < MAX_ENDERECOS_DIRETOS_INODE)
    {
        if (disco[blocoInode].inode.enderecosDiretos[i] != -1)
        {
            realocarBlocos(lista, disco[blocoInode].inode.enderecosDiretos[i]);
            disco[disco[blocoInode].inode.enderecosDiretos[i]].tipo = FREE;
            disco[blocoInode].inode.enderecosDiretos[i] = -1;
        }
        i++;
    }

    realocarBlocos(lista, blocoInode);
    disco[blocoInode].tipo = FREE;

    i = 2;
    while (i < disco[blocoAtual].diretorio.quantidadeEntradas &&
           strcmp(disco[blocoAtual].diretorio.entradas[i].nome, nome) != 0)
    {
        i++;
    }

    j = i;
    while (j < disco[blocoAtual].diretorio.quantidadeEntradas - 1)
    {
        disco[blocoAtual].diretorio.entradas[j] = disco[blocoAtual].diretorio.entradas[j + 1];
        j++;
    }
    disco[blocoAtual].diretorio.quantidadeEntradas--;

    printf("Link simbólico '%s' removido com sucesso\n", nome);
}

 
void iniciarTerminal(Bloco *disco, ListaBlocosLivres *listaBlocosLivres, int quantidadeBlocos)
{
    char linha[256];
    char *comando;
    char *arg1, *arg2;
    char caminho[100] = "/";
    int blocoAtual = 0; // diretório raiz

    do
    {
        printf("root@localhost:%s$ ", caminho);
        if (fgets(linha, sizeof(linha), stdin) == NULL)
            break;

        linha[strcspn(linha, "\n")] = '\0';
        comando = strtok(linha, " ");
        if (comando == NULL)
            continue;

        // ======== CD ========
        if (strcmp(comando, "cd") == 0)
        {
            arg1 = strtok(NULL, " ");
            if (!arg1)
            {
                printf("Uso: cd <caminho_diretorio>\n");
            }
            else if (strcmp(arg1, "/") == 0)
            {
                blocoAtual = 0;
                strcpy(caminho, "/");
            }
            else
            {
                size_t len = strlen(arg1);
                if (len > 0 && arg1[len - 1] == '/')
                    arg1[len - 1] = '\0';

                if (strcmp(arg1, "..") == 0)
                {
                    char *ultimaBarra = strrchr(caminho, '/');
                    if (ultimaBarra && ultimaBarra != caminho)
                        *ultimaBarra = '\0';
                    else
                        strcpy(caminho, "/");

                    int blocoPai = disco[blocoAtual].diretorio.entradas[1].bloco;
                    blocoAtual = blocoPai;
                }
                else
                {
                    // int blocoInodeDestino = localizarInodePorNome(arg1, blocoAtual, disco);
                    // // int blocoInodeDestino = abrirDiretorio(arg1, blocoAtual, disco);

                    // if (blocoInodeDestino == -1)
                    // {
                    //     printf("Diretório não encontrado: %s\n", arg1);
                    // }
                    // else if (disco[blocoInodeDestino].inode.tipo != 'd')
                    // {
                    //     printf("Erro: '%s' não é um diretório\n", arg1);
                    // }
                    // else if (!temPermissao(disco[blocoInodeDestino].inode, 'x'))
                    // {
                    //     printf("Permissão negada: sem permissão de execução neste diretório.\n");
                    // }
                    // else
                    // {
                    // int blocoDiretorio = disco[blocoInodeDestino].inode.enderecosDiretos[0];
                    int blocoDiretorio = abrirDiretorio(arg1, blocoAtual, disco);
                    if (blocoDiretorio >= 0)
                    {
                        if (!temPermissao(disco[blocoDiretorio].inode, 'x'))
                        {
                            printf("Permissão negada: sem permissão de execução neste diretório.\n");
                            continue;
                        }
                        blocoAtual = blocoDiretorio;

                        if (strcmp(caminho, "/") == 0)
                            snprintf(caminho, sizeof(caminho), "/%s", arg1);
                        else
                            snprintf(caminho + strlen(caminho),
                                     sizeof(caminho) - strlen(caminho), "/%s", arg1);
                    }
                    else
                    {
                        printf("Erro ao acessar diretório: %s\n", arg1);
                    }
                    //}
                }
            }
        }
        // ======== MKDIR ========
        else if (strcmp(comando, "mkdir") == 0)
        {
            arg1 = strtok(NULL, " ");
            if (!arg1)
            {
                printf("Uso: mkdir <nome_diretorio>\n");
                continue;
            }

            if (!temPermissao(disco[blocoAtual].inode, 'w'))
            {
                printf("Permissão negada: não é possível criar diretórios aqui.\n");
                continue;
            }

            inserirDiretorio(arg1, listaBlocosLivres, disco, blocoAtual);
        }

        // ======== TOUCH ========
        else if (strcmp(comando, "touch") == 0)
        {
            arg1 = strtok(NULL, " ");
            arg2 = strtok(NULL, " ");

            if (!arg1 || !arg2)
            {
                printf("Uso: touch <nome_arquivo> <tamanho_em_bytes>\n");
                continue;
            }

            if (!temPermissao(disco[blocoAtual].inode, 'w'))
            {
                printf("Permissão negada: não é possível criar arquivos neste diretório.\n");
                continue;
            }

            int tamanho = atoi(arg2);
            inserirArquivo(arg1, tamanho, listaBlocosLivres, disco, blocoAtual);
        }

        // ======== RM ========
        else if (strcmp(comando, "rm") == 0)
        {
            arg1 = strtok(NULL, " ");
            if (!arg1)
            {
                printf("Uso: rm <nome_arquivo>\n");
                continue;
            }

            if (!temPermissao(disco[blocoAtual].inode, 'w'))
            {
                printf("Permissão negada: não é possível remover arquivos aqui.\n");
                continue;
            }

            removerArquivo(arg1, blocoAtual, disco, listaBlocosLivres);
        }
        // ======== RMDIR ========
        else if (strcmp(comando, "rmdir") == 0)
        {
            arg1 = strtok(NULL, " ");
            if (!arg1)
            {
                printf("Uso: rm <nome_arquivo>\n");
                continue;
            }

            if (!temPermissao(disco[blocoAtual].inode, 'w'))
            {
                printf("Permissão negada: não é possível remover arquivos aqui.\n");
                continue;
            }

            removerDiretorio(arg1, blocoAtual, disco, listaBlocosLivres);
        }

        // ======== CHMOD ========
        else if (strcmp(comando, "chmod") == 0)
        {
            char *arg1 = strtok(NULL, " ");  // "+u" ou "-g"
            char *modos = strtok(NULL, " "); // "rw"
            char *nome = strtok(NULL, " ");  // nome do arquivo

            if (!arg1 || !modos || !nome)
            {
                printf("Uso: chmod (+|-) (u|g|o) (rwx) <arquivo>\n");
                continue;
            }

            char op = arg1[0];
            char esc = arg1[1];

            if ((op != '+' && op != '-') || (esc != 'u' && esc != 'g' && esc != 'o'))
            {
                printf("Formato inválido. Use chmod (+|-) (u|g|o) (rwx) <arquivo>\n");
                continue;
            }

            char operacao[2] = {op, '\0'};
            char escopo[2] = {esc, '\0'};

            alterarPermissao(operacao, escopo, modos, nome, blocoAtual, disco);
        }
        else if (strcmp(comando, "bad") == 0)
        {
            arg1 = strtok(NULL, " ");
            if (arg1 != NULL)
            {
                int numeroBloco = atoi(arg1);
                marcarBlocoBad(numeroBloco, disco, listaBlocosLivres, 1000);
            }
            else
            {
                printf("Uso: bad <numero_bloco>\n");
            }
        }
        else if (strcmp(comando, "relatorio3") == 0)
            relatorioArquivosCorrompidos(disco, quantidadeBlocos);
        else if (strcmp(comando, "df") == 0)
            mostrarEspacoDisco(disco, quantidadeBlocos, 10);
        else if (strcmp(comando, "vi") == 0)
        {
            arg1 = strtok(NULL, " ");
            if (arg1 != NULL)
            {
                visualizarArquivo(arg1, blocoAtual, disco, quantidadeBlocos);
            }
            else
                printf("Uso: vi <nome_arquivo>\n");
        }
        else if (strcmp(comando, "blocos") == 0)
        {
            arg1 = strtok(NULL, " ");
            if (arg1 != NULL)
            {
                blocosOcupadosArquivo(arg1, blocoAtual, disco);
            }
            else
            {
                printf("Uso: blocos <nome_arquivo>\n");
            }
        }
        else if (strcmp(comando, "estado") == 0)
        {
            imprimirEstadoBlocos(disco, quantidadeBlocos);
        }
        else if (strcmp(comando, "maior") == 0)
        {
            maiorArquivoPossivel(listaBlocosLivres);
        }
        else if (strcmp(comando, "link") == 0)
        {
            char *tipo = strtok(NULL, " ");
            char *origem = strtok(NULL, " ");
            char *destino = strtok(NULL, " ");

            if (!tipo || !origem || !destino)
            {
                printf("Uso: link (-s|-h) <origem> <destino>\n");
                continue;
            }

            if (!temPermissao(disco[blocoAtual].inode, 'w'))
            {
                printf("Permissão negada: não é possível criar links neste diretório.\n");
                continue;
            }

            if (strcmp(tipo, "-s") == 0)
            {
                criarLinkSimbolico(origem, destino, blocoAtual, listaBlocosLivres, disco);
            }
            else if (strcmp(tipo, "-h") == 0)
                printf("Hard link ainda não implementado\n");
            else
                printf("Tipo inválido. Use -s para simbólico ou -h para físico\n");
        }
        // ======== COMANDOS FORK (FILHO) ========
        else
        {
            pid_t pid = fork();
            if (pid < 0)
            {
                perror("Erro ao criar processo filho");
                continue;
            }
            else if (pid == 0)
            {
                if (strcmp(comando, "ls") == 0)
                {
                    arg1 = strtok(NULL, " ");
                    if (!temPermissao(disco[blocoAtual].inode, 'r'))
                    {
                        printf("Permissão negada: não é possível listar o diretório.\n");
                        _exit(0);
                    }

                    if (arg1 && strcmp(arg1, "-l") == 0)
                        listarDiretorioDetalhado(disco[blocoAtual].diretorio, disco);
                    else if (!arg1)
                        listarDiretorio(disco[blocoAtual].diretorio);
                    else
                        printf("Uso: ls [-l]\n");
                }
                else if (strcmp(comando, "vi") == 0)
                    printf("Simulação de vi (não implementada)\n");
                else if (strcmp(comando, "rmdir") == 0)
                    printf("Simulação de rmdir (não implementada)\n"); 
                else if (strcmp(comando, "unlink") == 0)
                {
                    {
                        char *tipo = strtok(NULL, " ");
                        char *nome = strtok(NULL, " ");

                        if (!tipo || !nome)
                        {
                            printf("Uso: unlink (-s|-h) <nome>\n");
                            _exit(0);
                        }

                        if (!temPermissao(disco[blocoAtual].inode, 'w'))
                        {
                            printf("Permissão negada: não é possível remover links aqui.\n");
                            _exit(0);
                        }

                        if (strcmp(tipo, "-s") == 0)
                            removerLinkSimbolico(nome, blocoAtual, disco, listaBlocosLivres);
                        else if (strcmp(tipo, "-h") == 0)
                            printf("Unlink hard link ainda não implementado\n");
                        else
                            printf("Tipo inválido. Use -s ou -h\n");
                    }
                }
                else
                    printf("Comando não encontrado: %s\n", comando);

                _exit(0);
            }
            else
            {
                wait(NULL);
            }
        }
    } while (strcmp(comando, "exit") != 0);
}

int main(void)
{
    // INFORMAR QUANTIDADE DE BLOCOS
    int quantidadeBlocos = 1000;

    clrscr();

    printf("Deseja informar a quantidade de blocos? [S/N] ");
    char op = getch();
    if (toupper(op) == 'S')
    {
        int qtde;
        fflush(stdin);
        printf("\nInforme a quantidade de blocos: \n");
        scanf("%d", &qtde);

        if (qtde > 1000 || qtde < 1)
        {
            printf("\nValor invalido sera setado 1000 blocos!\n");
        }
        else
        {
            quantidadeBlocos = qtde;
            printf("\n%d blocos definidos com sucesso!\n", quantidadeBlocos);
        }
    }
    else
    {
        printf("\n%d blocos definidos com sucesso!\n", quantidadeBlocos);
    }

    fflush(stdin);

    Bloco disco[quantidadeBlocos];

    // INICIALIZAR BLOCOS COMO LIVRES
    inicializarBlocos(disco, quantidadeBlocos);

    // Inicializalizar a lista de blocos livres
    ListaBlocosLivres *listaBlocosLivres = CriarListaBlocosLivres();
    inicializarListaBlocosLivres(listaBlocosLivres, quantidadeBlocos);

    // INICIALIZAR O DIRETORIO RAIZ
    int blocoRaiz = alocarBloco(listaBlocosLivres);
    inicializarDiretorio(disco, blocoRaiz, blocoRaiz);

    // gravar lista de blocos livres no disco
    gravarListaBlocosLivresDisco(listaBlocosLivres, disco);

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
    // inserirDiretorio("diretorio1", listaBlocosLivres, disco, blocoRaiz);
    // abrir diretorio
    // int blocoAtual = abrirDiretorio("diretorio1", blocoRaiz, disco);
    // removerArquivo("arquivo1.txt", blocoRaiz, disco, listaBlocosLivres);
    // inserirArquivo("arquivo_diretorio1.txt", 150, listaBlocosLivres, disco, blocoAtual);

    // ###### FIM INSERCOES DE TESTE #######
    iniciarTerminal(disco, listaBlocosLivres, quantidadeBlocos);

    // percorrer blocos e imprimir seus tipos
    for (int i = 0; i < quantidadeBlocos; i++)
    {
        Bloco b = disco[i];
        printf("Bloco %d: Tipo %c\n", i, disco[i].tipo);
    }

    return 0;
}