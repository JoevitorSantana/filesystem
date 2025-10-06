# 📁 Simulador de Sistema de Arquivos (I-Node)

Simula um sistema de arquivos Unix-like usando **I-Nodes** para gerenciar arquivos e diretórios. Cada comando executa em um processo filho via **`fork()`**.

---

## 🎯 Conceitos Chave

| Conceito                   | O que é                                                                                            | Por que usamos                                                                          |
| :------------------------- | :------------------------------------------------------------------------------------------------- | :-------------------------------------------------------------------------------------- |
| **I-Node**                 | Estrutura com metadados (permissões, tamanho, datas) e ponteiros para blocos do arquivo.           | Permite alocar arquivos de qualquer tamanho (direta, indireta simples, dupla e tripla). |
| **`fork()` / `wait()`**    | Cria processo filho para executar comando. O pai aguarda término.                                  | Simula o funcionamento de um shell real com processos separados.                        |
| **Blocos do Disco**        | Vetor de 1000 blocos × 10 bytes (configurável). Tipos: F (livre), B (bad), I (inode), A (arquivo). | Representa o espaço físico do disco de forma simplificada.                              |
| **Pilha de Blocos Livres** | Pilha que armazena números dos blocos disponíveis.                                                 | Alocação e liberação eficiente de espaço em disco.                                      |

---

## ⚙️ Tecnologias

- **Linguagem:** C puro
- **Estruturas:** `struct`, listas encadeadas, pilhas
- **Processos:** `fork()`, `wait()`

---

## ⌨️ Comandos Disponíveis

| Comando                     | Função                                        |
| :-------------------------- | :-------------------------------------------- |
| **`ls`** / **`ls -l`**      | Lista arquivos (simples ou com detalhes).     |
| **`mkdir <dir>`**           | Cria diretório.                               |
| **`rmdir <dir>`**           | Remove diretório vazio.                       |
| **`cd <dir>`**              | Navega entre diretórios (`.`, `..`, caminho). |
| **`touch <arq> <bytes>`**   | Cria arquivo e aloca blocos automaticamente.  |
| **`rm <arquivo>`**          | Deleta arquivo e libera seus blocos.          |
| **`vi <arquivo>`**          | Visualiza arquivo (verifica integridade).     |
| **`chmod (+)(-)ugo RWX`**   | Altera permissões (usuário/grupo/outros).     |
| **`link -h <orig> <dest>`** | Cria link físico (compartilha I-Node).        |
| **`link -s <orig> <dest>`** | Cria link simbólico (atalho).                 |
| **`unlink -h / -s`**        | Remove links.                                 |
| **`bad <número>`**          | Marca bloco como defeituoso.                  |
| **`df`**                    | Mostra espaço livre/ocupado em bytes.         |

---

## 📊 Relatórios

1. **Blocos ocupados** por arquivo específico
2. **Maior arquivo criável** no disco atual
3. **Arquivos corrompidos** (com blocos bad)
4. **Blocos perdidos** (não alocados nem livres)
5. **Estado do disco** (visualização completa)
6. **Explorador** (árvore de arquivos/diretórios)
7. **Árvore de diretórios** com atributos e inodes
8. **Links criados** (físicos e simbólicos)

---

## 🗂️ Estrutura do I-Node

**I-Node Principal:** 8 ponteiros

- 5 diretos → blocos do arquivo
- 1 indireto simples → aponta para I-Node com 5 ponteiros
- 1 indireto duplo → aponta para I-Node que aponta para outros I-Nodes
- 1 indireto triplo → 3 níveis de indireção

**I-Node de Extensão:** 5 ponteiros diretos
