#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ----------------------------------------
// definition
// ----------------------------------------
// token type
typedef enum {
    TK_RESERVED,    // 記号
    TK_NUM,         // 整数トークン
    TK_EOF          // 入力の終わりを表すトークン
} TokenKind;

typedef struct Token Token;

// トークン型
struct Token {
    TokenKind   kind;   // トークンの型
    Token*      next;   // 次の入力トークン
    int         val;    // kindがTK_NUMの場合、その数値
    char*       str;    // トークン文字列
};


typedef enum {
    ND_ADD,
    ND_SUB,
    ND_MUL,
    ND_DIV,
    ND_NUM,
} NodeKind;

typedef struct Node Node;

struct Node {
    NodeKind    kind;
    Node*       lhs;
    Node*       rhs;
    int         val;
};

// 現在着目しているトークン
Token*  token;
// 入力のバックアップ
char*   user_input;


// ----------------------------------------
// utility func
// ----------------------------------------

// エラー報告関数
void error(char *fmt, ...){
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

void error_at(char* loc, char* fmt, ...){
    va_list ap;
    va_start(ap, fmt);

    int pos = loc - user_input;
    fprintf(stderr, "%s\n", user_input);
    fprintf(stderr, "%*s", pos, "");
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}
// ----------------------------------------
// token func
// ----------------------------------------

// トークン試し読み関数
// 期待通りのトークン -> トークン消費
// 期待はずれのトークン -> なにもせずFALSEを返す。
bool tk_consume(char op){
    if (token->kind != TK_RESERVED | token->str[0] != op)
        return false;
    
    token = token->next;
    return true;
}

// トークンを読みすすめる。
// 予想外ならエラーにする。
void tk_expect(char op) {
    if (token->kind != TK_RESERVED || token->str[0] != op)
        error_at(token->str, "[error!] Next Character is not expected ! [%d]", op);
    
    token = token->next;
}

// 数値ならトークンを読みすすめる。
// 値ももらう
int tk_expect_number() {
    if (token->kind != TK_NUM)
        error_at(token->str, "[error] This is not a number.");
    
    int val = token->val;
    token = token->next;

    return val;
}

// トークン終端判定
bool tk_at_eof() {
    return token->kind == TK_EOF;
}

// 新しいトークンを作成してcurにつなげる
Token* tk_new_token(TokenKind kind, Token *cur, char *str){
    Token *tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->str = str;
    cur->next = tok;
    return tok;
}

Token*  tk_tokenize(){
    char* p = user_input;

    Token head;
    head.next = NULL;
    Token*  cur = &head;

    while(*p) {
        // 空白をスキップ
        if(isspace(*p)) {
            p++;
            continue;
        }

        if (strchr("+-*/()", *p)){
            cur = tk_new_token(TK_RESERVED, cur, p++);
            continue;
        }

        if (isdigit(*p)){
            cur = tk_new_token(TK_NUM, cur, p);
            cur->val = strtol(p, &p, 10);
            continue;
        }

        error_at(p, "[error!] Can not tokenize.");
    }

    tk_new_token(TK_EOF, cur, p);
    return head.next;
}

// ----------------------------------------
// parse func
// ----------------------------------------
Node* new_node(NodeKind kind){
    Node* node =calloc(1, sizeof(Node));
    node->kind = kind;
    return node;
}

Node* new_binary(NodeKind kind, Node* lhs, Node* rhs){
    Node* node = new_node(kind);
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

Node* new_node_num(int val){
    Node* node = new_node(ND_NUM);
    node->val = val;
    return node;
}


Node* expr();
Node* mul();
Node* unary();
Node* primary();

// expr = mul ("+" mul | "-" mul)
Node* expr() {
    Node* node = mul();

    for(;;){
        if(tk_consume('+'))
            node = new_binary(ND_ADD, node, mul());
        else if(tk_consume('-'))
            node = new_binary(ND_SUB, node, mul());
        else
            return node;
    }
}

// mul = unary ("*" unary | "/" unary)
Node* mul() {
    Node* node = unary();

    for(;;){
        if (tk_consume('*'))
            node = new_binary(ND_MUL, node, unary());
        else if (tk_consume('/'))
            node = new_binary(ND_DIV, node, unary());
        else
            return node;
    }
}

// unary = ("+" | "-")? primary
Node* unary() {
    if(tk_consume('+'))
        return unary();

    if(tk_consume('-'))
        return new_binary(ND_SUB, new_node_num(0), unary());

    return primary();
}

Node* primary() {
    if(tk_consume('(')) {
        Node* node = expr();
        tk_expect(')');
        return node;
    }

    return new_node_num(tk_expect_number());
}



// ----------------------------------------
// main func
// ----------------------------------------
void gen(Node* node) {
    if(node->kind == ND_NUM){
        printf("    push %d\n", node->val);
        return;
    }

    gen(node->lhs);
    gen(node->rhs);

    printf("    pop rdi\n");
    printf("    pop rax\n");

    switch(node->kind){
        case ND_ADD:
            printf("    add rax, rdi\n");
            break;
        case ND_SUB:
            printf("    sub rax, rdi\n");
            break;
        case ND_MUL:
            printf("    imul rax, rdi\n");
            break;
        case ND_DIV:
            printf("    cqo\n");
            printf("    idiv rdi\n");
            break;
    }

    printf("    push rax\n");
}


int main(int argc, char **argv){
    if(argc != 2){
        fprintf(stderr, "[error!] invalid args nums.");
        return 1;
    }

    user_input = argv[1];
    token = tk_tokenize();
    Node* node = expr();

    // アセンブリの前半部分
    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");

    gen(node);
    
    printf("    pop rax\n");
    printf("    ret\n");
    return 0;
}