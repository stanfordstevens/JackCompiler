//
//  main.c
//  JackCompiler
//
//  Created by Stanford Stevens on 7/28/17.
//  Copyright © 2017 Stanford Stevens. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <ctype.h>

#define isSymbol(c) c == '{' || c == '}' || c == '(' || c == ')' || c == '[' || c == ']' || c == '.' || c == ',' || c == ';' || c == '+' || c == '-' || c == '*' || c == '/' || c == '&' || c == '|' || c == '<' || c == '>' || c == '=' || c == '~'

typedef enum {
    TokenTypeSymbol,
    TokenTypeKeyword,
    TokenTypeInteger,
    TokenTypeString,
    TokenTypeIdentifier
} TokenType;

typedef struct Symbol {
    char *name;
    char *type;
    char *kind;
    //TODO: symbol index?? i dont understand how the index works for scope..
} Symbol;

size_t *number_of_class_symbols;
size_t *length_of_class_symbols;
Symbol **class_symbols;

size_t *number_of_sub_symbols;
size_t *length_of_sub_symbols;
Symbol **sub_symbols;

#pragma mark Symbol Table

void freeSymbolTable(Symbol **symbolTable, size_t *numberOfSymbols) {
    if (symbolTable) {
        for (int i = 0; i < *numberOfSymbols; i++) {
            Symbol *symbol = symbolTable[i];
            free(symbol->kind);
            free(symbol->name);
            free(symbol->type);
            free(symbol);
        }
        
        free(symbolTable);
    }
}

Symbol **add_symbol(Symbol **symbolTable, Symbol *symbol) {
    size_t *number_of_symbols = (symbolTable == class_symbols) ? number_of_class_symbols : number_of_sub_symbols;
    size_t *length_of_symbols = (symbolTable == class_symbols) ? length_of_class_symbols : length_of_class_symbols;
    
    *number_of_symbols = *number_of_symbols + 1;
    
    if (*number_of_symbols > *length_of_symbols) {
        *length_of_symbols = *length_of_symbols * 2;
        symbolTable = realloc(symbolTable, *length_of_symbols * sizeof(Symbol *));
    }
    
    size_t new_index = *number_of_symbols - 1;
    symbolTable[new_index] = symbol;
    
    return symbolTable;
}

Symbol **initialize_symbol_table(size_t *length_of_symbols, size_t *number_of_symbols) {
    int initial_symbol_count = 10;
    Symbol **symbolTable = malloc(initial_symbol_count*sizeof(Symbol));
    
    for (int i = 0; i < initial_symbol_count; i++) {
        symbolTable[i] = malloc(sizeof(Symbol *));
    }
    
    *length_of_symbols = initial_symbol_count;
    *number_of_symbols = 0;
    
    return symbolTable;
}

Symbol * _Nullable symbolWithName(char *name) {
    for (int i = 0; i < *number_of_sub_symbols; i++) {
        Symbol *symbol = sub_symbols[i];
        if (!strcmp(name, symbol->name)) {
            return symbol;
        }
    }
    
    for (int i = 0; i < *number_of_class_symbols; i++) {
        Symbol *symbol = class_symbols[i];
        if (!strcmp(name, symbol->name)) {
            return symbol;
        }
    }
     
    return NULL;
}

#pragma mark String Manipulations

char *trim_whitespace(char *string) {
    while(isspace((unsigned char)*string)) {
        string++;
    }
    
    if(*string == 0) { return string; }
    
    char *end = string + strlen(string) - 1;
    while(end > string && isspace((unsigned char)*end)) {
        end--;
    }
    
    *(end+1) = 0;
    
    return string;
}

char *pathWithInputPath(char *inputPath, char *extension) {
    char *path = malloc(strlen(inputPath) + 1);
    strcpy(path, inputPath);
    char *loc = strrchr(path, '.');
    *(loc) = 0;
    strcat(path, extension); //test files already have an xml file, must add extension to distinguish
    
    return path;
}

#pragma mark File Reading

int is_jack_file(const char *file) {
    const char *dot = strrchr(file, '.');
    if(!dot || dot == file) { return 0; }
    
    const char *extension = dot + 1;
    return (!strcmp(extension, "jack")) ? 1 : 0;
}

char *fgets_nl(char *buffer, int size, FILE *file) {
    buffer = fgets(buffer, size, file);
    if (buffer == NULL) { return NULL; }
    
    char *pos = strchr(buffer, '\n');
    if (pos != NULL) {
        *pos = '\0';
    }
    return buffer;
}

#pragma mark File Printing

void fputtabs(FILE *outputFile, int count) {
    while (count > 0) {
        fputc(' ', outputFile);
        fputc(' ', outputFile);
        count--;
    }
}

void fputsymbol(FILE *outputFile, Symbol *symbol, int tabCount) {
    fputtabs(outputFile, tabCount);
    fprintf(outputFile, "name: %s, kind: %s, type: %s\n", symbol->name, symbol->kind, symbol->type);
}

void fputterminal(char *terminal, char *tag, int tabCount, FILE *outputFile) {
    fputtabs(outputFile, tabCount);
    fprintf(outputFile, "<%s> %s </%s>\n", tag, terminal, tag);
}

TokenType tokenType(char *token) {
    if (isSymbol(token[0])) {
        return TokenTypeSymbol;
    } else if (!strcmp(token, "class") || !strcmp(token, "constructor") || !strcmp(token, "function") || !strcmp(token, "method") ||
               !strcmp(token, "field") || !strcmp(token, "static") || !strcmp(token, "var") || !strcmp(token, "int") ||
               !strcmp(token, "char") || !strcmp(token, "boolean") || !strcmp(token, "void") || !strcmp(token, "true") ||
               !strcmp(token, "false") || !strcmp(token, "null") || !strcmp(token, "this") || !strcmp(token, "let") ||
               !strcmp(token, "do") || !strcmp(token, "if") || !strcmp(token, "else") || !strcmp(token, "while") ||
               !strcmp(token, "return")) {
        return TokenTypeKeyword;
    } else if (token[0] >= '0' && token[0] <= '9') {
        return TokenTypeInteger;
    } else if (token[0] == '"') {
        return TokenTypeString;
    } else {
        return TokenTypeIdentifier;
    }
}

#pragma mark Compile Functions

void compileVarBody(FILE *inputFile, FILE *outputFile, int tabCount, Symbol *newSymbol, Symbol **symbolTable) {
    char line[256];
    
    fgets_nl(line, sizeof(line), inputFile);
    TokenType lineType = tokenType(line);
    if (lineType == TokenTypeIdentifier || !strcmp(line, "int") || !strcmp(line, "char") || !strcmp(line, "boolean")) {
        fputterminal(line, (lineType == TokenTypeIdentifier) ? "identifier" : "keyword", tabCount, outputFile);
        
        newSymbol->type = malloc(strlen(line));
        strcpy(newSymbol->type, line);
    } else {
        printf("Var declaration does not have a valid type!\n");
        exit(1);
    }
    
    while (1) {
        fgets_nl(line, sizeof(line), inputFile);
        if (tokenType(line) == TokenTypeIdentifier) {
            newSymbol->name = malloc(strlen(line));
            strcpy(newSymbol->name, line);
            
            fputsymbol(outputFile, newSymbol, tabCount);
        } else {
            printf("Var name must be of token type 'identifier'!\n");
            exit(1);
        }
        
        fgets_nl(line, sizeof(line), inputFile);
        if (!strcmp(line, ";") || !strcmp(line, ",")) {
            fputterminal(line, "symbol", tabCount, outputFile);
            
            if (!strcmp(line, ";")) {
                break;
            } else {
                char *kind = newSymbol->kind;
                char *type = newSymbol->type;
                
                newSymbol = malloc(sizeof(Symbol));
                newSymbol->kind = malloc(strlen(kind));
                strcpy(newSymbol->kind, kind);
                newSymbol->type = malloc(strlen(type));
                strcpy(newSymbol->type, type);
                
                add_symbol(symbolTable, newSymbol);
            }
        } else {
            printf("Expected ';' at end of line of var declaration(s)!\n");
            exit(1);
        }
    }
}

void compileClassVarDeclaration(char *varType, FILE *inputFile, FILE *outputFile, int tabCount) {
    int outerTabCount = tabCount;
    int innerTabCount = tabCount + 1;
    
    fputtabs(outputFile, outerTabCount);
    fputs("<classVarDec>\n", outputFile);
    
    fputterminal(varType, "keyword", innerTabCount, outputFile);
    
    Symbol *newSymbol = malloc(sizeof(Symbol));
    newSymbol->kind = malloc(strlen(varType));
    strcpy(newSymbol->kind, varType);
    class_symbols = add_symbol(class_symbols, newSymbol);
    
    compileVarBody(inputFile, outputFile, innerTabCount, newSymbol, class_symbols);
    
    fputtabs(outputFile, outerTabCount);
    fputs("</classVarDec>\n", outputFile);
}

void compileVarDeclaration(FILE *inputFile, FILE *outputFile, int tabCount) {
    int outerTabCount = tabCount;
    int innerTabCount = tabCount + 1;
    
    fputtabs(outputFile, outerTabCount);
    fputs("<varDec>\n", outputFile);
    
    fputterminal("var", "keyword", innerTabCount, outputFile);
    
    Symbol *newSymbol = malloc(sizeof(Symbol));
    char *kind = "var";
    newSymbol->kind = malloc(strlen(kind));
    strcpy(newSymbol->kind, kind);
    sub_symbols = add_symbol(sub_symbols, newSymbol);
    
    compileVarBody(inputFile, outputFile, innerTabCount, newSymbol, sub_symbols);
    
    fputtabs(outputFile, outerTabCount);
    fputs("</varDec>\n", outputFile);
}

void compileParameterList(FILE *inputFile, FILE *outputFile, int tabCount) {
    char line[256];
    int outerTabCount = tabCount;
    int innerTabCount = tabCount + 1;
    
    fputtabs(outputFile, outerTabCount);
    fputs("<parameterList>\n", outputFile);
    
    while (1) {
        fpos_t pos;
        fgetpos(inputFile, &pos);
        
        fgets_nl(line, sizeof(line), inputFile);
        if (!strcmp(line, ",")) {
            fputterminal(line, "symbol", innerTabCount, outputFile);
        } else if (!strcmp(line, ")")) {
            fsetpos(inputFile, &pos);
            break;
        } else {
            Symbol *newSymbol = malloc(sizeof(Symbol));
            char *kind = "argument";
            newSymbol->kind = malloc(strlen(kind));
            strcpy(newSymbol->kind, kind);
            sub_symbols = add_symbol(sub_symbols, newSymbol);
            
            TokenType lineType = tokenType(line);
            if (lineType == TokenTypeIdentifier || !strcmp(line, "int") || !strcmp(line, "char") || !strcmp(line, "boolean") || !strcmp(line, "void")) {
                fputterminal(line, (lineType == TokenTypeIdentifier) ? "identifier" : "keyword", innerTabCount, outputFile);
                
                newSymbol->type = malloc(strlen(line));
                strcpy(newSymbol->type, line);
            } else {
                printf("Subroutine parameter does not have a valid type!\n");
                exit(1);
            }
            
            fgets_nl(line, sizeof(line), inputFile);
            if (tokenType(line) == TokenTypeIdentifier) {
                newSymbol->name = malloc(strlen(line));
                strcpy(newSymbol->name, line);
                
                fputsymbol(outputFile, newSymbol, innerTabCount);
            } else {
                printf("Class var name must be of token type 'identifier'!\n");
                exit(1);
            }
        }
    }
    
    fputtabs(outputFile, outerTabCount);
    fputs("</parameterList>\n", outputFile);
}

void compileExpression(FILE *inputFile, FILE *outputFile, int tabCount);

void compileExpressionList(FILE *inputFile, FILE *outputFile, int tabCount) {
    char line[256];
    int outerTabCount = tabCount;
    int innerTabCount = tabCount + 1;
    
    fputtabs(outputFile, outerTabCount);
    fputs("<expressionList>\n", outputFile);
    
    while (1) {
        fpos_t pos;
        fgetpos(inputFile, &pos);
        
        fgets_nl(line, sizeof(line), inputFile);
        if (!strcmp(line, ")")) {
            fputtabs(outputFile, outerTabCount);
            fputs("</expressionList>\n", outputFile);
            
            fsetpos(inputFile, &pos);
            break;
        } else if (!strcmp(line, ",")) {
            fputterminal(",", "symbol", innerTabCount, outputFile);
        } else {
            fsetpos(inputFile, &pos);
            compileExpression(inputFile, outputFile, innerTabCount);
        }
    }
}

void compileSubroutineCall(FILE *inputFile, FILE *outputFile, int tabCount) {
    char line[256];
    
    fgets_nl(line, sizeof(line), inputFile);
    if (tokenType(line) == TokenTypeIdentifier) {
        fputterminal(line, "identifier", tabCount, outputFile);
    } else {
        printf("Expected identifier at beginning of subroutine call!\n");
        exit(1);
    }
    
    fgets_nl(line, sizeof(line), inputFile);
    if (!strcmp(line, "(")) {
        fputterminal("(", "symbol", tabCount, outputFile);
        
        compileExpressionList(inputFile, outputFile, tabCount);

        fgets_nl(line, sizeof(line), inputFile);
        if (!strcmp(line, ")")) {
            fputterminal(")", "symbol", tabCount, outputFile);
        } else {
            printf("Expected ')' to end expression list!\n");
            exit(1);
        }
    } else if (!strcmp(line, ".")) {
        fputterminal(".", "symbol", tabCount, outputFile);
        
        fgets_nl(line, sizeof(line), inputFile);
        if (tokenType(line) == TokenTypeIdentifier) {
            fputterminal(line, "identifier", tabCount, outputFile);
        } else {
            printf("Invalid subroutine name!\n");
            exit(1);
        }
        
        fgets_nl(line, sizeof(line), inputFile);
        if (!strcmp(line, "(")) {
            fputterminal("(", "symbol", tabCount, outputFile);
            
            compileExpressionList(inputFile, outputFile, tabCount);
            
            fgets_nl(line, sizeof(line), inputFile);
            if (!strcmp(line, ")")) {
                fputterminal(")", "symbol", tabCount, outputFile);
            } else {
                printf("Expected ')' to end expression list!\n");
                exit(1);
            }
        } else {
            printf("Invalid subroutine name!\n");
            exit(1);
        }
    } else {
        printf("Expected '(' or '.' after subroutine call!\n");
        exit(1);
    }
}

void compileTerm(FILE *inputFile, FILE *outputFile, int tabCount) {
    char line[256];
    int outerTabCount = tabCount;
    int innerTabCount = tabCount + 1;
    
    fputtabs(outputFile, outerTabCount);
    fputs("<term>\n", outputFile);
    
    fpos_t initialTermPos;
    fgetpos(inputFile, &initialTermPos);
    
    fgets_nl(line, sizeof(line), inputFile);
    TokenType termType = tokenType(line);
    switch (termType) {
        case TokenTypeString:
            fputtabs(outputFile, innerTabCount);
            fputs("<stringConstant> ", outputFile);
            memmove(line, line+1, strlen(line));
            fputs(line, outputFile);
            fseek(outputFile, -1, SEEK_CUR);
            fputs("</stringConstant>\n", outputFile);
            break;
        case TokenTypeInteger:
            fputterminal(line, "integerConstant", innerTabCount, outputFile);
            break;
        case TokenTypeKeyword:
            fputterminal(line, "keyword", innerTabCount, outputFile);
            break;
        case TokenTypeIdentifier:
        {
            fpos_t pos;
            fgetpos(inputFile, &pos);
            
            fgets_nl(line, sizeof(line), inputFile);
            fsetpos(inputFile, &initialTermPos); //TODO: this is weird and hacky
            if (!strcmp(line, "[")) {
                fgets_nl(line, sizeof(line), inputFile);
                
                Symbol *symbol = symbolWithName(line);
                if (symbol) {
                    fputsymbol(outputFile, symbol, innerTabCount);
                } else {
                    fputterminal(line, "identifier", innerTabCount, outputFile);
                }
                
                fgets_nl(line, sizeof(line), inputFile);
                fputterminal("[", "symbol", innerTabCount, outputFile);
                
                compileExpression(inputFile, outputFile, innerTabCount);
                
                fgets_nl(line, sizeof(line), inputFile);
                if (!strcmp(line, "]")) {
                    fputterminal("]", "symbol", innerTabCount, outputFile);
                } else {
                    printf("Expected ']' to end expression!\n");
                    exit(1);
                }
            } else if (!strcmp(line, "(") || !strcmp(line, ".")) {
                compileSubroutineCall(inputFile, outputFile, innerTabCount);
            } else {
                fgets_nl(line, sizeof(line), inputFile);
                
                Symbol *symbol = symbolWithName(line);
                if (symbol) {
                    fputsymbol(outputFile, symbol, innerTabCount);
                } else {
                    fputterminal(line, "identifier", innerTabCount, outputFile);
                }
                
                fsetpos(inputFile, &pos);
            }
            break;
        }
        case TokenTypeSymbol:
            if (!strcmp(line, "(")) {
                fputterminal("(", "symbol", innerTabCount, outputFile);
                
                compileExpression(inputFile, outputFile, innerTabCount);
                
                fgets_nl(line, sizeof(line), inputFile);
                if (!strcmp(line, ")")) {
                    fputterminal(")", "symbol", innerTabCount, outputFile);
                } else {
                    printf("Expected ')' to end expression!\n");
                    exit(1);
                }
            } else if (!strcmp(line, "-") || !strcmp(line, "~")) {
                fputterminal(line, "symbol", innerTabCount, outputFile);
                
                compileTerm(inputFile, outputFile, innerTabCount);
            }
            break;
        default:
            printf("Invalid token type!\n");
            exit(1);
            break;
    }
    
    fputtabs(outputFile, outerTabCount);
    fputs("</term>\n", outputFile);
}

void compileExpression(FILE *inputFile, FILE *outputFile, int tabCount) {
    char line[256];
    int outerTabCount = tabCount;
    int innerTabCount = tabCount + 1;
    
    fputtabs(outputFile, outerTabCount);
    fputs("<expression>\n", outputFile);
    
    while (1) {
        compileTerm(inputFile, outputFile, innerTabCount);
        
        fpos_t pos;
        fgetpos(inputFile, &pos);
        
        fgets_nl(line, sizeof(line), inputFile);
        if (!strcmp(line, "+") || !strcmp(line, "-") || !strcmp(line, "*") || !strcmp(line, "/") || !strcmp(line, "&amp;") || !strcmp(line, "|") || !strcmp(line, "&lt;") || !strcmp(line, "&gt;") || !strcmp(line, "=")) {
            fputterminal(line, "symbol", innerTabCount, outputFile);
        } else {
            fsetpos(inputFile, &pos);
            break;
        }
    }
    
    fputtabs(outputFile, outerTabCount);
    fputs("</expression>\n", outputFile);
}

void compileStatements(FILE *inputFile, FILE *outputFile, int tabCount) {
    char line[256];
    int outerTabCount = tabCount;
    int innerTabCount = tabCount + 1;
    
    fputtabs(outputFile, outerTabCount);
    fputs("<statements>\n", outputFile);
    
    while (1) {
        fpos_t pos;
        fgetpos(inputFile, &pos);
        
        fgets_nl(line, sizeof(line), inputFile);
        if (!strcmp(line, "let") || !strcmp(line, "if") || !strcmp(line, "while") || !strcmp(line, "do") || !strcmp(line, "return")) {
            char *statementType = malloc(strlen(line));
            strcpy(statementType, line);
            
            fputtabs(outputFile, innerTabCount);
            fprintf(outputFile, "<%sStatement>\n", statementType);
            
            int newInnerTab = innerTabCount + 1;
            
            fputterminal(statementType, "keyword", newInnerTab, outputFile);
            
            if (!strcmp(line, "let")) {
                fgets_nl(line, sizeof(line), inputFile);
                if (tokenType(line) == TokenTypeIdentifier) {
                    Symbol *symbol = symbolWithName(line);
                    if (symbol) {
                        fputsymbol(outputFile, symbol, newInnerTab);
                    } else {
                        fputterminal(line, "identifier", newInnerTab, outputFile);
                    }
                } else {
                    printf("Local var name must be of token type 'identifier'!\n");
                    exit(1);
                }
                
                fgets_nl(line, sizeof(line), inputFile);
                if (!strcmp(line, "[")) {
                    fputterminal("[", "symbol", newInnerTab, outputFile);
                    
                    compileExpression(inputFile, outputFile, newInnerTab);
                    
                    fgets_nl(line, sizeof(line), inputFile);
                    if (!strcmp(line, "]")) {
                        fputterminal("]", "symbol", newInnerTab, outputFile);
                    } else {
                        printf("Expected ']' to end expression!\n");
                        exit(1);
                    }
                    
                    fgets_nl(line, sizeof(line), inputFile);
                }
                
                if (!strcmp(line, "=")) {
                    fputterminal("=", "symbol", newInnerTab, outputFile);
                } else {
                    printf("Expected '=' after let statement declaration!\n");
                    exit(1);
                }
                
                compileExpression(inputFile, outputFile, newInnerTab);
                
                fgets_nl(line, sizeof(line), inputFile);
                if (!strcmp(line, ";")) {
                    fputterminal(";", "symbol", newInnerTab, outputFile);
                } else {
                    printf("Expected ';' at end of 'let' statement!\n");
                    exit(1);
                }
            } else if (!strcmp(line, "if") || !strcmp(line, "while")) {
                fgets_nl(line, sizeof(line), inputFile);
                if (!strcmp(line, "(")) {
                    fputterminal("(", "symbol", newInnerTab, outputFile);
                } else {
                    printf("Expected '(' at beginning of %s expression!\n", statementType);
                    exit(1);
                }
                
                compileExpression(inputFile, outputFile, newInnerTab);
                
                fgets_nl(line, sizeof(line), inputFile);
                if (!strcmp(line, ")")) {
                    fputterminal(")", "symbol", newInnerTab, outputFile);
                } else {
                    printf("Expected ')' at end of %s expression!\n", statementType);
                    exit(1);
                }
                
                fgets_nl(line, sizeof(line), inputFile);
                if (!strcmp(line, "{")) {
                    fputterminal("{", "symbol", newInnerTab, outputFile);
                } else {
                    printf("Expected '{' at beginning of %s statement!\n", statementType);
                    exit(1);
                }
                
                compileStatements(inputFile, outputFile, newInnerTab);
                
                fgets_nl(line, sizeof(line), inputFile);
                if (!strcmp(line, "}")) {
                    fputterminal("}", "symbol", newInnerTab, outputFile);
                } else {
                    printf("Expected '}' at end of %s statement!\n", statementType);
                    exit(1);
                }
                
                if (!strcmp(statementType, "if")) {
                    fpos_t pos;
                    fgetpos(inputFile, &pos);
                    
                    fgets_nl(line, sizeof(line), inputFile);
                    if (!strcmp(line, "else")) {
                        fputterminal("else", "keyword", newInnerTab, outputFile);
                        
                        fgets_nl(line, sizeof(line), inputFile);
                        if (!strcmp(line, "{")) {
                            fputterminal("{", "symbol", newInnerTab, outputFile);
                        } else {
                            printf("Expected '{' at beginning of 'else' statement!\n");
                            exit(1);
                        }
                        
                        compileStatements(inputFile, outputFile, newInnerTab);
                        
                        fgets_nl(line, sizeof(line), inputFile);
                        if (!strcmp(line, "}")) {
                            fputterminal("}", "symbol", newInnerTab, outputFile);
                        } else {
                            printf("Expected '}' at end of 'else' statement!\n");
                            exit(1);
                        }
                    } else {
                        fsetpos(inputFile, &pos);
                    }
                }
            } else if (!strcmp(line, "do")) {
                compileSubroutineCall(inputFile, outputFile, newInnerTab);
                
                fgets_nl(line, sizeof(line), inputFile);
                if (!strcmp(line, ";")) {
                    fputterminal(";", "symbol", newInnerTab, outputFile);
                } else {
                    printf("Expected ';' at end of 'do' statement!\n");
                    exit(1);
                }
            } else if (!strcmp(line, "return")) {
                fpos_t pos;
                fgetpos(inputFile, &pos);
                
                fgets_nl(line, sizeof(line), inputFile);
                if (strcmp(line, ";") != 0) {
                    fsetpos(inputFile, &pos);
                    compileExpression(inputFile, outputFile, newInnerTab);
                    
                    fgets_nl(line, sizeof(line), inputFile);
                }
                
                if (!strcmp(line, ";")) {
                    fputterminal(";", "symbol", newInnerTab, outputFile);
                } else {
                    printf("Expected ';' at end of 'return' statement!\n");
                    exit(1);
                }
            }
            
            fputtabs(outputFile, innerTabCount);
            fprintf(outputFile, "</%sStatement>\n", statementType);
            free(statementType);
        } else if (!strcmp(line, "}")) {
            fsetpos(inputFile, &pos);
            break;
        } else {
            printf("Not a valid statement type: %s\n", line);
            exit(1);
        }
    }
    
    fputtabs(outputFile, outerTabCount);
    fputs("</statements>\n", outputFile);
}

void compileSubroutineBody(FILE *inputFile, FILE *outputFile, int tabCount) {
    char line[256];
    int outerTabCount = tabCount;
    int innerTabCount = tabCount + 1;
    
    fputtabs(outputFile, outerTabCount);
    fputs("<subroutineBody>\n", outputFile);
    
    fgets_nl(line, sizeof(line), inputFile);
    if (!strcmp(line, "{")) {
        fputterminal("{", "symbol", innerTabCount, outputFile);
    } else {
        printf("Subroutine Body should begin with '{'!\n");
        exit(1);
    }

    while (1) {
        fpos_t pos;
        fgetpos(inputFile, &pos);
        
        fgets_nl(line, sizeof(line), inputFile);
        if (!strcmp(line, "var")) {
            compileVarDeclaration(inputFile, outputFile, innerTabCount);
        } else if (!strcmp(line, "}")) {
            fputterminal("}", "symbol", innerTabCount, outputFile);
            break;
        } else if (!strcmp(line, "let") || !strcmp(line, "if") || !strcmp(line, "while") || !strcmp(line, "do") || !strcmp(line, "return")) {
            fsetpos(inputFile, &pos);
            compileStatements(inputFile, outputFile, innerTabCount);
        } else {
            printf("Unrecognized keyword specified in class!\n");
            exit(1);
        }
    }
    
    fputtabs(outputFile, outerTabCount);
    fputs("</subroutineBody>\n", outputFile);
}

void compileSubroutineDeclaration(char *subType, FILE *inputFile, FILE *outputFile, int tabCount) {
    char line[256];
    int outerTabCount = tabCount;
    int innerTabCount = tabCount + 1;
    
    //reinitialize sub_symbols because new subroutine is being compiled
    freeSymbolTable(sub_symbols, number_of_sub_symbols);
    sub_symbols = initialize_symbol_table(length_of_sub_symbols, number_of_sub_symbols);
    
    fputtabs(outputFile, outerTabCount);
    fputs("<subroutineDec>\n", outputFile);
    
    fputterminal(subType, "keyword", innerTabCount, outputFile);
    
    fgets_nl(line, sizeof(line), inputFile);
    TokenType lineType = tokenType(line);
    if (lineType == TokenTypeIdentifier || !strcmp(line, "int") || !strcmp(line, "char") || !strcmp(line, "boolean") || !strcmp(line, "void")) {
        fputterminal(line, (lineType == TokenTypeIdentifier) ? "identifier" : "keyword", innerTabCount, outputFile);
    } else {
        printf("Class subroutine declaration does not have a valid return type!\n");
        exit(1);
    }
    
    fgets_nl(line, sizeof(line), inputFile);
    if (tokenType(line) == TokenTypeIdentifier) {
        fputterminal(line, "identifier", innerTabCount, outputFile);
    } else {
        printf("Class subroutine name must have a valid name!\n");
        exit(1);
    }
    
    fgets_nl(line, sizeof(line), inputFile);
    if (!strcmp(line, "(")) {
        fputterminal("(", "symbol", innerTabCount, outputFile);
    } else {
        printf("Class subroutine missing '('!\n");
        exit(1);
    }
    
    compileParameterList(inputFile, outputFile, innerTabCount);
    
    fgets_nl(line, sizeof(line), inputFile);
    if (!strcmp(line, ")")) {
        fputterminal(")", "symbol", innerTabCount, outputFile);
    } else {
        printf("Class subroutine missing ')' at end of parameter list!\n");
        exit(1);
    }
    
    compileSubroutineBody(inputFile, outputFile, innerTabCount);
    
    fputtabs(outputFile, outerTabCount);
    fputs("</subroutineDec>\n", outputFile);
}

void compileClass(FILE *inputFile, FILE *outputFile) {
    char line[256];
    int tabCount = 1;
    
    class_symbols = initialize_symbol_table(length_of_class_symbols, number_of_class_symbols);
    
    fgets_nl(line, sizeof(line), inputFile);
    if (!strcmp(line, "class")) {
        fputs("<class>\n", outputFile);
        fputterminal("class", "keyword", tabCount, outputFile);
    } else {
        printf("File does not begin with a class declaration!\n");
        exit(1);
    }
    
    fgets_nl(line, sizeof(line), inputFile);
    if (tokenType(line) == TokenTypeIdentifier) {
        fputterminal(line, "identifier", tabCount, outputFile);
    } else {
        printf("Class declaration has no class name!\n");
        exit(1);
    }
    
    fgets_nl(line, sizeof(line), inputFile);
    if (!strcmp(line, "{")) {
        fputterminal("{", "symbol", tabCount, outputFile);
    } else {
        printf("Class declaration is missing '{'!\n");
        exit(1);
    }
    
    while (fgets_nl(line, sizeof(line), inputFile)) {
        if (!strcmp(line, "field") || !strcmp(line, "static")) {
            compileClassVarDeclaration(line, inputFile, outputFile, tabCount);
        } else if (!strcmp(line, "constructor") || !strcmp(line, "function") || !strcmp(line, "method")) {
            compileSubroutineDeclaration(line, inputFile, outputFile, tabCount);
        } else if (!strcmp(line, "}")) {
            fputterminal("}", "symbol", tabCount, outputFile);
        } else {
            printf("Unrecognized keyword specified in class!\n");
            exit(1);
        }
    }
    
    fputs("</class>\n", outputFile);
}

#pragma mark Main

int main(int argc, const char * argv[]) {
    char *filepath = malloc(200);
    printf("Enter filepath bitch> ");
    scanf("%s", filepath);
    
    filepath = trim_whitespace(filepath);
    
    struct stat path_stat;
    stat(filepath, &path_stat);
    int number_of_files = 0;
    char **files = malloc(sizeof(char *));
    
    if (S_ISDIR(path_stat.st_mode)) {
        DIR *directory;
        struct dirent *entry;
        
        directory = opendir(filepath);
        if (directory != NULL) {
            while ((entry = readdir(directory))) {
                if (!is_jack_file(entry->d_name)) { continue; }
                
                number_of_files++;
                files = realloc(files, number_of_files * sizeof(char *));
                
                size_t new_index = number_of_files - 1;
                char *entry_path = malloc(strlen(filepath) + strlen(entry->d_name) + 1 + 1);
                strcpy(entry_path, filepath);
                strcat(entry_path, "/");
                strcat(entry_path, entry->d_name);
                size_t entry_length = strlen(entry_path) + 1;
                files[new_index] = malloc(entry_length);
                strcpy(files[new_index], entry_path);
                free(entry_path);
            }
            
            closedir(directory);
        }
    } else if (S_ISREG(path_stat.st_mode) && is_jack_file(filepath)) {
        number_of_files = 1;
        files[0] = malloc(strlen(filepath) + 1);
        strcpy(files[0], filepath);
    }
    
    free(filepath);
    
    if (number_of_files == 0) {
        printf("No jack files found\n");
        return 1;
    }

    for (int i = 0; i < number_of_files; i++) {
        //set up input file for reading
        char *inputPath = files[i];
        FILE *inputFile = fopen(inputPath, "r");
        
        //set up helper file for writing
        char *helperPath = pathWithInputPath(inputPath, "_helperFile.xml");
        FILE *helperFile = fopen(helperPath, "w+");
        
        //tokenizer
        char line[256];
        while (fgets(line, sizeof(line), inputFile)) {
            char *trimmed = trim_whitespace(line);
            
            if ((trimmed[0] == '/' && (trimmed[1] == '/' || trimmed[1] == '*')) || trimmed[0] == '*' ||
                isspace(trimmed[0]) || !strcmp(trimmed, "") || !strcmp(trimmed, "\r\n")) { continue; }
            
            int printingString = 0;
            for (int i = 0; i < strlen(trimmed); i++) {
                char c = trimmed[i];
                
                fseek(helperFile, -1, SEEK_CUR);
                char previous = fgetc(helperFile);
                
                if (printingString && c != '"') {
                    fputc(c, helperFile);
                } else if (c == '/' && i+1 < strlen(trimmed) && trimmed[i+1] == '/') {
                    break; //skip rest of line
                } else if (isSymbol(c)) {
                    if (previous != '\n') {
                        fputc('\n', helperFile);
                    }
                    
                    if (c == '<') {
                        fputs("&lt;", helperFile);
                    } else if (c == '>') {
                        fputs("&gt;", helperFile);
                    } else if (c == '&') {
                        fputs("&amp;", helperFile);
                    } else {
                        fputc(c, helperFile);
                    }
                    
                    fputc('\n', helperFile);
                } else if (c == '"') {
                    if (!printingString && previous != '\n') {
                        fputc('\n', helperFile);
                    }
                    
                    fputc(c, helperFile);
                    printingString = !printingString;
                } else if (c == ' ') {
                    if (previous != '\n') {
                        fputc('\n', helperFile);
                    }
                } else {
                    fputc(c, helperFile);
                }
            }
        }
        
        //set up output file for writing
        char *outputPath = pathWithInputPath(inputPath, "_stanOutput.xml");
        FILE *outputFile = fopen(outputPath, "w");
        
        //initialize symbol table counts
        length_of_class_symbols = malloc(sizeof(size_t));
        length_of_sub_symbols = malloc(sizeof(size_t));
        number_of_class_symbols = malloc(sizeof(size_t));
        number_of_sub_symbols = malloc(sizeof(size_t));
        
        *length_of_class_symbols = 0;
        *length_of_sub_symbols = 0;
        *number_of_class_symbols = 0;
        *number_of_sub_symbols = 0;
        
        //parse
        rewind(helperFile);
        compileClass(helperFile, outputFile);
        
        //cleanup
        freeSymbolTable(class_symbols, number_of_class_symbols);
        class_symbols = NULL;
        
        freeSymbolTable(sub_symbols, number_of_sub_symbols);
        sub_symbols = NULL;
        
        free(length_of_class_symbols);
        free(length_of_sub_symbols);
        free(number_of_class_symbols);
        free(number_of_sub_symbols);
        
        fclose(outputFile);
        
        fclose(helperFile);
        remove(helperPath);
    }
    
    free(files);
    
    return 0;
}
