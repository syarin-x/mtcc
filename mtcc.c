#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

// 現在着目しているトークン
Token*  token;


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
        error("[error!] Next Character is not expected ! [%d]", op);
    
    token = token->next;
}

// 数値ならトークンを読みすすめる。
// 値ももらう
int tk_expect_number() {
    if (token->kind != TK_NUM)
        error("[error] This is not a number.");
    
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

Token*  tk_tokenize(char* p){
    Token head;
    head.next = NULL;
    Token*  cur = &head;

    while(*p) {
        // 空白をスキップ
        if(isspace(*p)) {
            p++;
            continue;
        }

        if (*p == '+' || *p == '-'){
            cur = tk_new_token(TK_RESERVED, cur, p++);
            continue;
        }

        if (isdigit(*p)){
            cur = tk_new_token(TK_NUM, cur, p);
            cur->val = strtol(p, &p, 10);
            continue;
        }

        error("[error!] Can not tokenize.");
    }

    tk_new_token(TK_EOF, cur, p);
    return head.next;
}

int main(int argc, char **argv){
    if(argc != 2){
        fprintf(stderr, "[error!] invalid args nums.");
        return 1;
    }

    // トーカナイズする
    token = tk_tokenize(argv[1]);

    // アセンブリの前半部分
    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");

    // 式の最初
    printf("    mov rax, %d\n", tk_expect_number());

    while(!tk_at_eof()){
        if(tk_consume('+')){
            printf("    add rax, %d\n", tk_expect_number());
            continue;
        }

        tk_expect('-');
        printf("    sub rax, %d\n", tk_expect_number());
    }

    printf("    ret\n");
    return 0;
}