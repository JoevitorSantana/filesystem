#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <conio.c>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>

#define MAX_ENDERECOS_DIRETOS_INODE 5
#define MAX_ENDERECOS_INDIRETOS_SIMPLES_INODE 5
#define MAX_ENDERECOS_INDIRETOS_DUPLO_INODE 5
#define MAX_ENDERECOS_INDIRETOS_TRIPLO_INODE 5
#define MAX_ENDERECOS_BLOCOS_LISTA_LIVRE 10

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

void inicializarDiretorio(Diretorio &diretorio)
{
    diretorio.quantidadeEntradas = 0;
    for (int i = 0; i < 10; i++)
    {
        diretorio.entradas[i].nome[0] = '\0';
        diretorio.entradas[i].bloco = -1;
    }
}

void inicializarBlocos(Bloco *disco, int quantidadeBlocos)
{
    for (int i = 0; i < quantidadeBlocos; i++)
    {
        disco[i].tipo = FREE;
        inicializarDiretorio(disco[i].diretorio);
        inicializarInode(&disco[i].inode);
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
        {
            printf("Espaco insuficiente\n");
            return -1;
        }
    }
    else if (Count(Aux->blocos) == 0 && Aux->prox == NULL)
    {
        printf("Espaco insuficiente\n");
        return -1;
    }

    int bloco = Pop(Aux->blocos);
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

    while (quantidade > 0 && Aux != NULL)
    {
        // verificar se a pilha de blocos do no atual nao esta vazia
        while (quantidade > 0 && Count(Aux->blocos) > 0)
        {
            // if (quantidadeAlocada < MAX_ENDERECOS_DIRETOS_INODE) {
            int bloco = Pop(Aux->blocos);
            inode->enderecosDiretos[quantidadeBlocos - quantidade] = bloco;
            disco[bloco].tipo = ARQUIVO;
            // printf("Bloco alocado: %d\n", bloco);
            // } else if (quantidadeBlocos > MAX_ENDERECOS_DIRETOS_INODE && quantidadeBlocos <= (MAX_ENDERECOS_DIRETOS_INODE + MAX_ENDERECOS_INDIRETOS_SIMPLES_INODE))
            // {   // determinar se vai usar bloco de enderecos simples, duplo ou triplo
            //     // alocar bloco de enderecos simples
            //     if (disco[enderecoBlocoInode].inode.enderecoSimplesIndireto == -1) {
            //         int blocoEnderecoSimples = alocarBloco(lista);
            //         disco[enderecoBlocoInode].inode.enderecoSimplesIndireto = blocoEnderecoSimples;
            //         disco[blocoEnderecoSimples].tipo = INODE;
            //         disco[blocoEnderecoSimples].inodeEnderecoSimples.quantidadeEnderecos = 0;
            //     }
            //     int bloco = Pop(Aux->blocos);
            //     disco[disco[enderecoBlocoInode].inode.enderecoSimplesIndireto].inodeEnderecoSimples.enderecos[disco[disco[enderecoBlocoInode].inode.enderecoSimplesIndireto].inodeEnderecoSimples.quantidadeEnderecos++] = bloco;
            //     disco[bloco].tipo = ARQUIVO;
            // } else if (quantidadeBlocos > (MAX_ENDERECOS_DIRETOS_INODE + MAX_ENDERECOS_INDIRETOS_SIMPLES_INODE) && quantidadeBlocos < MAX_ENDERECOS_DIRETOS_INODE * MAX_ENDERECOS_INDIRETOS_DUPLO_INODE)
            // {   // Nó duplo indireto
            //     // alocar bloco de enderecos duplos
            //     if (disco[enderecoBlocoInode].inode.enderecosDuploIndireto == -1) {
            //         int blocoEnderecoDuplo = alocarBloco(lista);
            //         disco[enderecoBlocoInode].inode.enderecosDuploIndireto = blocoEnderecoDuplo;
            //         disco[blocoEnderecoDuplo].tipo = INODE;
            //         disco[blocoEnderecoDuplo].inodeEnderecoDuplo.quantidadeEnderecos = 0;
            //     }
            //     // alocar bloco de enderecos simples dentro do bloco de enderecos duplos
            //     if (disco[disco[enderecoBlocoInode].inode.enderecosDuploIndireto].inodeEnderecoDuplo.quantidadeEnderecos == 0 ||
            //         disco[disco[enderecoBlocoInode].inode.enderecosDuploIndireto].inodeEnderecoDuplo.enderecos[disco[disco[enderecoBlocoInode].inode.enderecosDuploIndireto].inodeEnderecoDuplo.quantidadeEnderecos - 1] == MAX_ENDERECOS_INDIRETOS_SIMPLES_INODE)
            //     {
            //         int blocoEnderecoSimples = alocarBloco(lista);
            //         disco[disco[enderecoBlocoInode].inode.enderecosDuploIndireto].inodeEnderecoDuplo.enderecos[disco[disco[enderecoBlocoInode].inode.enderecosDuploIndireto].inodeEnderecoDuplo.quantidadeEnderecos++] = blocoEnderecoSimples;
            //         disco[blocoEnderecoSimples].tipo = INODE;
            //         disco[blocoEnderecoSimples].inodeEnderecoSimples.quantidadeEnderecos = 0;
            //     }
            //     int bloco = Pop(Aux->blocos);
            //     int blocoSimplesAtual = disco[disco[enderecoBlocoInode].inode.enderecosDuploIndireto].inodeEnderecoDuplo.enderecos[disco[disco[enderecoBlocoInode].inode.enderecosDuploIndireto].inodeEnderecoDuplo.quantidadeEnderecos - 1];
            //     disco[blocoSimplesAtual].inodeEnderecoSimples.enderecos[disco[blocoSimplesAtual].inodeEnderecoSimples.quantidadeEnderecos++] = bloco;
            //     disco[bloco].tipo = ARQUIVO;
            // }

            quantidade--;
            // quantidadeAlocada++;
        }
        // se a pilha de blocos do no atual estiver vazia, ir para o proximo no
        if (Count(Aux->blocos) == 0)
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
            disco[disco[blocoInodeArquivo].inode.enderecosDiretos[j]].tipo = FREE;
            disco[blocoInodeArquivo].inode.enderecosDiretos[j] = -1;
        }
    }
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
    ;
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
    for (int i = 2; i < disco[blocoAtual].diretorio.quantidadeEntradas; i++)
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

void testarPermissaoCHMOD(int blocoRaiz, ListaBlocosLivres *listaBlocosLivres, Bloco disco[])
{
    printf("\n===== TESTES AUTOMÁTICOS DE PERMISSÕES =====\n\n");

    // Criar inode para o diretório raiz para poder testar permissões
    int blocoInodeRaiz = alocarBloco(listaBlocosLivres);
    disco[blocoInodeRaiz].tipo = INODE;
    inicializarInode(&disco[blocoInodeRaiz].inode);

    // Adicionar entrada "." apontando para o inode do diretório raiz
    disco[blocoRaiz].diretorio.entradas[0].bloco = blocoInodeRaiz;
    disco[blocoInodeRaiz].inode.enderecosDiretos[0] = blocoRaiz;

    // 1. Cria diretórios e arquivos de teste
    inserirDiretorio("testeDir", listaBlocosLivres, disco, blocoRaiz);
    inserirArquivo("testeFile.txt", 10, listaBlocosLivres, disco, blocoRaiz);

    printf("-> Estado inicial:\n");
    listarDiretorioDetalhado(disco[blocoRaiz].diretorio, disco);

    // 2. Remove permissão de escrita do dono (u-w)
    printf("\n>> chmod -u w testeDir\n");
    alterarPermissao("-", "u", "w", "testeDir", blocoRaiz, disco);
    listarDiretorioDetalhado(disco[blocoRaiz].diretorio, disco);

    // 3. Tenta criar um arquivo dentro do diretório sem permissão de escrita
    printf("\n>> Tentando criar arquivo em testeDir (sem w):\n");
    int blocoTesteDir = abrirDiretorio("testeDir", blocoRaiz, disco);
    int blocoInodeTesteDir = localizarInodePorNome("testeDir", blocoRaiz, disco);
    if (temPermissao(disco[blocoInodeTesteDir].inode, 'w'))
    {
        inserirArquivo("ok.txt", 10, listaBlocosLivres, disco, blocoTesteDir);
        printf("✅ Criado com sucesso (w presente)\n");
    }
    else
    {
        printf("🚫 Permissão negada (sem w no diretório)\n");
    }

    // 4. Remove permissão de execução e tenta cd
    printf("\n>> chmod -u x testeDir\n");
    alterarPermissao("-", "u", "x", "testeDir", blocoRaiz, disco);
    listarDiretorioDetalhado(disco[blocoRaiz].diretorio, disco);

    printf("\n>> Tentando cd testeDir (sem x):\n");
    blocoInodeTesteDir = localizarInodePorNome("testeDir", blocoRaiz, disco);
    if (temPermissao(disco[blocoInodeTesteDir].inode, 'x'))
    {
        abrirDiretorio("testeDir", blocoRaiz, disco);
        printf("✅ cd permitido\n");
    }
    else
    {
        printf("🚫 Permissão negada (sem x)\n");
    }

    // 5. Adiciona permissão novamente e tenta entrar
    printf("\n>> chmod +u x testeDir\n");
    alterarPermissao("+", "u", "x", "testeDir", blocoRaiz, disco);
    listarDiretorioDetalhado(disco[blocoRaiz].diretorio, disco);

    printf("\n>> Tentando cd testeDir novamente:\n");
    blocoInodeTesteDir = localizarInodePorNome("testeDir", blocoRaiz, disco);
    if (temPermissao(disco[blocoInodeTesteDir].inode, 'x'))
    {
        abrirDiretorio("testeDir", blocoRaiz, disco);
        printf("✅ cd permitido novamente (x restaurado)\n");
    }
    else
    {
        printf("🚫 Ainda bloqueado\n");
    }

    // 6. Modifica arquivo
    printf("\n>> chmod -u r testeFile.txt\n");
    alterarPermissao("-", "u", "r", "testeFile.txt", blocoRaiz, disco);
    listarDiretorioDetalhado(disco[blocoRaiz].diretorio, disco);

    printf("\n>> Tentando ler testeFile.txt (sem r):\n");
    if (temPermissao(disco[localizarInodePorNome("testeFile.txt", blocoRaiz, disco)].inode, 'r'))
    {
        printf("✅ Pode ler arquivo\n");
    }
    else
    {
        printf("🚫 Sem permissão de leitura\n");
    }

    // 7. Restaura leitura e testa novamente
    printf("\n>> chmod +u r testeFile.txt\n");
    alterarPermissao("+", "u", "r", "testeFile.txt", blocoRaiz, disco);
    listarDiretorioDetalhado(disco[blocoRaiz].diretorio, disco);

    printf("\n>> Tentando ler testeFile.txt (com r restaurado):\n");
    if (temPermissao(disco[localizarInodePorNome("testeFile.txt", blocoRaiz, disco)].inode, 'r'))
    {
        printf("✅ Pode ler arquivo\n");
    }
    else
    {
        printf("🚫 Sem permissão de leitura\n");
    }

    // 8. Testa remoção de arquivo sem permissão de escrita no diretório
    printf("\n>> Removendo permissão w do diretório raiz:\n");
    printf(">> chmod -u w (diretório raiz)\n");
    disco[blocoInodeRaiz].inode.protecao[1] = '-'; // Remove w do root
    printf("Permissões do diretório raiz atualizadas: %s\n", disco[blocoInodeRaiz].inode.protecao);

    printf("\n>> Tentando remover testeFile.txt (sem w no diretório raiz):\n");
    if (!temPermissao(disco[blocoInodeRaiz].inode, 'w'))
    {
        printf("🚫 Permissão negada: não é possível remover arquivos aqui.\n");
    }
    else
    {
        removerArquivo("testeFile.txt", blocoRaiz, disco, listaBlocosLivres);
        printf("✅ Arquivo removido\n");
    }

    // 9. Restaura permissão de escrita no diretório raiz
    printf("\n>> Restaurando permissão w do diretório raiz\n");
    printf(">> chmod +u w (diretório raiz)\n");
    disco[blocoInodeRaiz].inode.protecao[1] = 'w'; // Restaura w do user
    printf("Permissões do diretório raiz restauradas: %s\n", disco[blocoInodeRaiz].inode.protecao);

    // 10. Cria novo diretório e arquivo para mais testes
    printf("\n>> Criando testeDir2 e arquivo2.txt\n");
    inserirDiretorio("testeDir2", listaBlocosLivres, disco, blocoRaiz);
    inserirArquivo("arquivo2.txt", 20, listaBlocosLivres, disco, blocoRaiz);
    listarDiretorioDetalhado(disco[blocoRaiz].diretorio, disco);

    // 11. Remove todas as permissões do usuário
    printf("\n>> chmod -u rwx arquivo2.txt\n");
    alterarPermissao("-", "u", "rwx", "arquivo2.txt", blocoRaiz, disco);
    listarDiretorioDetalhado(disco[blocoRaiz].diretorio, disco);

    printf("\n>> Verificando permissões de arquivo2.txt:\n");
    int blocoInodeArquivo2 = localizarInodePorNome("arquivo2.txt", blocoRaiz, disco);
    printf("   r: %s\n", temPermissao(disco[blocoInodeArquivo2].inode, 'r') ? "✅" : "🚫");
    printf("   w: %s\n", temPermissao(disco[blocoInodeArquivo2].inode, 'w') ? "✅" : "🚫");
    printf("   x: %s\n", temPermissao(disco[blocoInodeArquivo2].inode, 'x') ? "✅" : "🚫");

    // 12. Adiciona todas as permissões de volta
    printf("\n>> chmod +u rwx arquivo2.txt\n");
    alterarPermissao("+", "u", "rwx", "arquivo2.txt", blocoRaiz, disco);
    listarDiretorioDetalhado(disco[blocoRaiz].diretorio, disco);

    printf("\n>> Verificando permissões de arquivo2.txt:\n");
    printf("   r: %s\n", temPermissao(disco[blocoInodeArquivo2].inode, 'r') ? "✅" : "🚫");
    printf("   w: %s\n", temPermissao(disco[blocoInodeArquivo2].inode, 'w') ? "✅" : "🚫");
    printf("   x: %s\n", temPermissao(disco[blocoInodeArquivo2].inode, 'x') ? "✅" : "🚫");

    // 13. Testa permissões de grupo (g)
    printf("\n>> chmod -g rx testeDir2\n");
    alterarPermissao("-", "g", "rx", "testeDir2", blocoRaiz, disco);
    listarDiretorioDetalhado(disco[blocoRaiz].diretorio, disco);

    // 14. Testa permissões de outros (o)
    printf("\n>> chmod -o rwx testeDir2\n");
    alterarPermissao("-", "o", "rwx", "testeDir2", blocoRaiz, disco);
    listarDiretorioDetalhado(disco[blocoRaiz].diretorio, disco);

    // 15. Restaura todas as permissões
    printf("\n>> chmod +g rx testeDir2\n");
    alterarPermissao("+", "g", "rx", "testeDir2", blocoRaiz, disco);
    printf(">> chmod +o rwx testeDir2\n");
    alterarPermissao("+", "o", "rwx", "testeDir2", blocoRaiz, disco);
    listarDiretorioDetalhado(disco[blocoRaiz].diretorio, disco);

    // 16. Teste final: mkdir sem permissão
    printf("\n>> Removendo permissão w do diretório raiz\n");
    printf(">> chmod -u w (diretório raiz)\n");
    disco[blocoInodeRaiz].inode.protecao[1] = '-'; // Remove w
    printf("Permissões do diretório raiz: %s\n", disco[blocoInodeRaiz].inode.protecao);

    printf("\n>> Tentando criar diretório testeDir3 (sem w):\n");
    if (!temPermissao(disco[blocoInodeRaiz].inode, 'w'))
    {
        printf("🚫 Permissão negada: não é possível criar diretórios aqui.\n");
    }
    else
    {
        inserirDiretorio("testeDir3", listaBlocosLivres, disco, blocoRaiz);
        printf("✅ Diretório criado\n");
    }

    // Restaura permissão
    printf("\n>> Restaurando permissão w do diretório raiz\n");
    printf(">> chmod +u w (diretório raiz)\n");
    disco[blocoInodeRaiz].inode.protecao[1] = 'w'; // Restaura w
    printf("Permissões do diretório raiz: %s\n", disco[blocoInodeRaiz].inode.protecao);

    printf("\n>> Tentando criar diretório testeDir3 (com w):\n");
    if (temPermissao(disco[blocoInodeRaiz].inode, 'w'))
    {
        inserirDiretorio("testeDir3", listaBlocosLivres, disco, blocoRaiz);
        printf("✅ Diretório criado com sucesso\n");
    }
    else
    {
        printf("🚫 Permissão negada\n");
    }

    listarDiretorioDetalhado(disco[blocoRaiz].diretorio, disco);

    printf("\n===== FIM DOS TESTES =====\n\n");
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

void testarLinkSimbolico(int blocoRaiz, ListaBlocosLivres *listaBlocosLivres, Bloco disco[])
{
    printf("\n===== TESTES AUTOMÁTICOS DE LINKS SIMBÓLICOS =====\n\n");

    // Prepara ambiente básico
    inserirDiretorio("diretorio1", listaBlocosLivres, disco, blocoRaiz);
    inserirDiretorio("dirLink", listaBlocosLivres, disco, blocoRaiz);
    // Cria arquivo para ser a origem (30 bytes -> 3 blocos de conteúdo no seu FS)
    inserirArquivo("arquivoBase.txt", 30, listaBlocosLivres, disco, blocoRaiz);
    // Cria um arquivo grande para testar o uso de múltiplos blocos no link simbólico
    inserirArquivo("arqGrande.txt", 150, listaBlocosLivres, disco, blocoRaiz);
    listarDiretorioDetalhado(disco[blocoRaiz].diretorio, disco);

    // 2️⃣ Cria link simbólico para o arquivo
    printf("\n>> Criando link simbólico 'atalho.txt' -> 'arquivoBase.txt'\n");
    criarLinkSimbolico("arquivoBase.txt", "atalho.txt", blocoRaiz, listaBlocosLivres, disco);
    listarDiretorioDetalhado(disco[blocoRaiz].diretorio, disco);

    // 3️⃣ Verifica se o link foi criado e se as permissões foram copiadas
    int blocoLink = localizarInodePorNome("atalho.txt", blocoRaiz, disco);
    int blocoOrigem = localizarInodePorNome("arquivoBase.txt", blocoRaiz, disco);

    if (blocoLink != -1 && disco[blocoLink].inode.tipo == 'l')
        printf("✅ Link simbólico criado com sucesso (atalho.txt)\n");
    else
        printf("🚫 Falha ao criar link simbólico\n");

    if (blocoLink != -1 && blocoOrigem != -1)
    {
        if (strcmp(disco[blocoLink].inode.protecao, disco[blocoOrigem].inode.protecao) == 0)
            printf("✅ Permissões copiadas corretamente.\n");
        else
            printf("🚫 Falha ao copiar permissões. Link: %s | Origem: %s\n",
                   disco[blocoLink].inode.protecao, disco[blocoOrigem].inode.protecao);
    }

    // 4️⃣ Testa criar link simbólico com nome duplicado
    printf("\n>> Tentando criar link simbólico com nome já existente (atalho.txt)\n");
    criarLinkSimbolico("arquivoBase.txt", "atalho.txt", blocoRaiz, listaBlocosLivres, disco);

    // 5️⃣ Testa criar link simbólico para origem inexistente
    printf("\n>> Tentando criar link simbólico para origem inexistente ('naoExiste.txt')\n");
    criarLinkSimbolico("naoExiste.txt", "atalhoFantasma", blocoRaiz, listaBlocosLivres, disco);

    // 6️⃣ Cria link simbólico para diretório
    printf("\n>> Criando link simbólico 'atalhoDir' -> 'dirLink'\n");
    criarLinkSimbolico("dirLink", "atalhoDir", blocoRaiz, listaBlocosLivres, disco);
    listarDiretorioDetalhado(disco[blocoRaiz].diretorio, disco);

    // 7️⃣ Cria link simbólico com string de origem longa (testa múltiplos blocos de conteúdo do link)
    printf("\n>> Criando link simbólico 'atalhoLongo' -> 'arqGrande.txt'\n");
    criarLinkSimbolico("arqGrande.txt", "atalhoLongo", blocoRaiz, listaBlocosLivres, disco);
    listarDiretorioDetalhado(disco[blocoRaiz].diretorio, disco);

    // 8️⃣ Remove links simbólicos
    printf("\n>> Removendo link simbólico 'atalho.txt'\n");
    removerLinkSimbolico("atalho.txt", blocoRaiz, disco, listaBlocosLivres);
    printf("\n>> Removendo link simbólico 'atalhoDir'\n");
    removerLinkSimbolico("atalhoDir", blocoRaiz, disco, listaBlocosLivres);
    printf("\n>> Removendo link simbólico 'atalhoLongo'\n");
    removerLinkSimbolico("atalhoLongo", blocoRaiz, disco, listaBlocosLivres);
    listarDiretorioDetalhado(disco[blocoRaiz].diretorio, disco);

    // 9️⃣ Tenta remover link inexistente
    printf("\n>> Tentando remover link simbólico inexistente 'naoExiste'\n");
    removerLinkSimbolico("naoExiste", blocoRaiz, disco, listaBlocosLivres);

    // 🔟 Cria e remove múltiplos links seguidos (testa desalocação e reuso)
    printf("\n>> Criando vários links simbólicos de teste\n");
    criarLinkSimbolico("arquivoBase.txt", "link1", blocoRaiz, listaBlocosLivres, disco);
    criarLinkSimbolico("arquivoBase.txt", "link2", blocoRaiz, listaBlocosLivres, disco);
    criarLinkSimbolico("arquivoBase.txt", "link3", blocoRaiz, listaBlocosLivres, disco);
    listarDiretorioDetalhado(disco[blocoRaiz].diretorio, disco);

    printf("\n>> Removendo links criados\n");
    removerLinkSimbolico("link1", blocoRaiz, disco, listaBlocosLivres);
    removerLinkSimbolico("link2", blocoRaiz, disco, listaBlocosLivres);
    removerLinkSimbolico("link3", blocoRaiz, disco, listaBlocosLivres);
    listarDiretorioDetalhado(disco[blocoRaiz].diretorio, disco);

    // 1️⃣1️⃣ Testa erro de espaço insuficiente (simulado) - deve capturar o erro
    printf("\n>> Testando erro de espaço insuficiente (simulado)\n");
    ListaBlocosLivres listaVazia = {0};
    criarLinkSimbolico("arquivoBase.txt", "semEspaco", blocoRaiz, &listaVazia, disco);

    printf("\n===== FIM DOS TESTES DE LINKS SIMBÓLICOS =====\n\n");
}

void iniciarTerminal(Bloco *disco, ListaBlocosLivres *listaBlocosLivres)
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
                    int blocoInodeDestino = localizarInodePorNome(arg1, blocoAtual, disco);

                    if (blocoInodeDestino == -1)
                    {
                        printf("Diretório não encontrado: %s\n", arg1);
                    }
                    else if (disco[blocoInodeDestino].inode.tipo != 'd')
                    {
                        printf("Erro: '%s' não é um diretório\n", arg1);
                    }
                    else if (!temPermissao(disco[blocoInodeDestino].inode, 'x'))
                    {
                        printf("Permissão negada: sem permissão de execução neste diretório.\n");
                    }
                    else
                    {
                        int blocoDiretorio = disco[blocoInodeDestino].inode.enderecosDiretos[0];
                        if (blocoDiretorio >= 0)
                        {
                            blocoAtual = blocoDiretorio;

                            if (strcmp(caminho, "/") == 0)
                                sprintf(caminho, "/%s", arg1);
                            else
                            {
                                strcat(caminho, "/");
                                strcat(caminho, arg1);
                            }
                        }
                        else
                        {
                            printf("Erro ao acessar diretório: %s\n", arg1);
                        }
                    }
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

        // ======== CHMOD ========
        else if (strcmp(comando, "chmod") == 0)
        {
            char *arg1 = strtok(NULL, " ");
            char *modos = strtok(NULL, " ");
            char *nome = strtok(NULL, " ");

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

        // ======== LINK SIMBÓLICO (MOVIMENTO E ADAPTAÇÃO) ========
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

            // 🛑 CHECAGEM DE PERMISSÃO DE ESCRITA NO DIRETÓRIO ATUAL
            if (!temPermissao(disco[blocoAtual].inode, 'w'))
            {
                printf("Permissão negada: não é possível criar links neste diretório.\n");
                continue;
            }
            // 🛑 FIM DA CHECAGEM DE PERMISSÃO

            if (strcmp(tipo, "-s") == 0)
            {
                // A função criarLinkSimbolico já fará as checagens internas (origem existe, destino não existe)
                criarLinkSimbolico(origem, destino, blocoAtual, listaBlocosLivres, disco);
            }
            else if (strcmp(tipo, "-h") == 0)
                printf("Hard link ainda não implementado\n");
            else
                printf("Tipo inválido. Use -s para simbólico ou -h para físico\n");
        }
        // ======== FIM LINK SIMBÓLICO ========

        // ======== COMANDOS EXECUTADOS NO FILHO ========
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

                // ======== UNLINK SIMBÓLICO (MOVIMENTO PARA FILHO) ========
                else if (strcmp(comando, "unlink") == 0)
                {
                    char *tipo = strtok(NULL, " ");
                    char *nome = strtok(NULL, " ");

                    if (!tipo || !nome)
                    {
                        printf("Uso: unlink (-s|-h) <nome>\n");
                        _exit(0);
                    }

                    // Checagem de permissão de escrita para remover
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
                // ======== FIM UNLINK SIMBÓLICO ========

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
    // int quantidadeBlocos = 20;
    int quantidadeBlocos = 1000;

    // printf("Informe a quantidade de blocos: ");
    // scanf("%d", &quantidadeBlocos);

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
    inserirArquivo("arquivo1.txt", 25, listaBlocosLivres, disco, blocoRaiz);
    // inserirArquivo("arquivo2.txt", 25, listaBlocosLivres, disco, blocoRaiz);
    // inserirArquivo("arquivo3.txt", 25, listaBlocosLivres, disco, blocoRaiz);
    // inserirArquivo("arquivo4.txt", 25, listaBlocosLivres, disco, blocoRaiz);
    // inserirArquivo("arquivo5.txt", 25, listaBlocosLivres, disco, blocoRaiz);
    inserirDiretorio("diretorio1", listaBlocosLivres, disco, blocoRaiz);
    // abrir diretorio
    int blocoAtual = abrirDiretorio("diretorio1", blocoRaiz, disco);
    removerArquivo("arquivo1.txt", blocoRaiz, disco, listaBlocosLivres);
    inserirArquivo("arquivo_diretorio1.txt", 150, listaBlocosLivres, disco, blocoAtual);

    // testarPermissaoCHMOD(blocoRaiz, listaBlocosLivres, disco);
    testarLinkSimbolico(blocoRaiz, listaBlocosLivres, disco);

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