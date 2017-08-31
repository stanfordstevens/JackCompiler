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

int is_jack_file(const char *file) {
    const char *dot = strrchr(file, '.');
    if(!dot || dot == file) { return 0; }
    
    const char *extension = dot + 1;
    return (!strcmp(extension, "jack")) ? 1 : 0;
}

char* trim_whitespace(char *string) {
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

TokenType tokenType(char *token) {
    if (isSymbol(token[0])) {
        return TokenTypeSymbol;
    } else if (!strcmp(token, "class\n") || !strcmp(token, "constructor\n") || !strcmp(token, "function\n") || !strcmp(token, "method\n") ||
               !strcmp(token, "field\n") || !strcmp(token, "static\n") || !strcmp(token, "var\n") || !strcmp(token, "int\n") ||
               !strcmp(token, "char\n") || !strcmp(token, "boolean\n") || !strcmp(token, "void\n") || !strcmp(token, "true\n") ||
               !strcmp(token, "false\n") || !strcmp(token, "null\n") || !strcmp(token, "this\n") || !strcmp(token, "let\n") ||
               !strcmp(token, "do\n") || !strcmp(token, "if\n") || !strcmp(token, "else\n") || !strcmp(token, "while\n") ||
               !strcmp(token, "return\n")) {
        return TokenTypeKeyword;
    } else if (token[0] >= '0' && token[0] <= '9') {
        return TokenTypeInteger;
    } else if (token[0] == '"') {
        return TokenTypeString;
    } else {
        return TokenTypeIdentifier;
    }
}

void fputtabs(FILE *outputFile, int count) {
    while (count > 0) {
        fputc(' ', outputFile);
        fputc(' ', outputFile);
        count--;
    }
}

void compileVarBody(FILE *inputFile, FILE *outputFile, int tabCount) {
    char line[256];
    
    fgets(line, sizeof(line), inputFile); //TODO: make function that gets line and removes new line character??
    TokenType lineType = tokenType(line);
    if (lineType == TokenTypeIdentifier || !strcmp(line, "int\n") || !strcmp(line, "char\n") || !strcmp(line, "boolean\n")) {
        char *type = malloc(11);
        type = (lineType == TokenTypeIdentifier) ? "identifier" : "keyword"; //TODO: ask daniel why it crashes when i try to free this
        
        fputtabs(outputFile, tabCount);
        fprintf(outputFile, "<%s> %s", type, line);
        fseek(outputFile, -1, SEEK_CUR);
        fprintf(outputFile, " </%s>\n", type);
    } else {
        printf("Var declaration does not have a valid type!\n");
        exit(1);
    }
    
    while (1) {
        fgets(line, sizeof(line), inputFile);
        if (tokenType(line) == TokenTypeIdentifier) {
            fputtabs(outputFile, tabCount);
            fputs("<identifier> ", outputFile);
            fputs(line, outputFile);
            fseek(outputFile, -1, SEEK_CUR);
            fputs(" </identifier>\n", outputFile);
        } else {
            printf("Var name must be of token type 'identifier'!\n");
            exit(1);
        }
        
        fgets(line, sizeof(line), inputFile);
        if (!strcmp(line, ";\n") || !strcmp(line, ",\n")) {
            fputtabs(outputFile, tabCount);
            fputs("<symbol> ", outputFile);
            fputs(line, outputFile);
            fseek(outputFile, -1, SEEK_CUR);
            fputs(" </symbol>\n", outputFile);
            
            if (!strcmp(line, ";\n")) {
                break;
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
    
    fputtabs(outputFile, innerTabCount);
    fputs("<keyword> ", outputFile);
    fputs(varType, outputFile);
    fseek(outputFile, -1, SEEK_CUR);
    fputs(" </keyword>\n", outputFile);
    
    compileVarBody(inputFile, outputFile, innerTabCount);
    
    fputtabs(outputFile, outerTabCount);
    fputs("</classVarDec>\n", outputFile);
}

void compileVarDeclaration(FILE *inputFile, FILE *outputFile, int tabCount) {
    int outerTabCount = tabCount;
    int innerTabCount = tabCount + 1;
    
    fputtabs(outputFile, outerTabCount);
    fputs("<varDec>\n", outputFile);
    
    fputtabs(outputFile, innerTabCount);
    fputs("<keyword> ", outputFile);
    fputs("var", outputFile);
    fputs(" </keyword>\n", outputFile);
    
    compileVarBody(inputFile, outputFile, innerTabCount);
    
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
        
        fgets(line, sizeof(line), inputFile);
        if (!strcmp(line, ",\n")) {
            fputtabs(outputFile, innerTabCount);
            fputs("<symbol> ", outputFile);
            fputs(line, outputFile);
            fseek(outputFile, -1, SEEK_CUR);
            fputs(" </symbol>\n", outputFile);
        } else if (!strcmp(line, ")\n")) {
            fsetpos(inputFile, &pos);
            break;
        } else {
            TokenType lineType = tokenType(line);
            if (lineType == TokenTypeIdentifier || !strcmp(line, "int\n") || !strcmp(line, "char\n") || !strcmp(line, "boolean\n") || !strcmp(line, "void\n")) { //TODO: remove dupe
                char *type = malloc(11);
                type = (lineType == TokenTypeIdentifier) ? "identifier" : "keyword";
                
                fputtabs(outputFile, innerTabCount);
                fprintf(outputFile, "<%s> %s", type, line);
                fseek(outputFile, -1, SEEK_CUR);
                fprintf(outputFile, " </%s>\n", type);
            } else {
                printf("Subroutine parameter does not have a valid type!\n");
                exit(1);
            }
            
            fgets(line, sizeof(line), inputFile);
            if (tokenType(line) == TokenTypeIdentifier) {
                fputtabs(outputFile, innerTabCount);
                fputs("<identifier> ", outputFile);
                fputs(line, outputFile);
                fseek(outputFile, -1, SEEK_CUR);
                fputs(" </identifier>\n", outputFile);
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
        
        fgets(line, sizeof(line), inputFile);
        if (!strcmp(line, ")\n")) {
            fputtabs(outputFile, outerTabCount);
            fputs("</expressionList>\n", outputFile);
            
            fsetpos(inputFile, &pos);
            break;
        } else if (!strcmp(line, ",\n")) {
            fputtabs(outputFile, innerTabCount);
            fputs("<symbol> , </symbol>\n", outputFile);
        } else {
            fsetpos(inputFile, &pos);
            compileExpression(inputFile, outputFile, innerTabCount);
        }
    }
}

void compileTerm(FILE *inputFile, FILE *outputFile, int tabCount) {
    char line[256];
    int outerTabCount = tabCount;
    int innerTabCount = tabCount + 1;
    
    fputtabs(outputFile, outerTabCount);
    fputs("<term>\n", outputFile);
    
    fgets(line, sizeof(line), inputFile);
    TokenType termType = tokenType(line);
    switch (termType) {
        case TokenTypeString:
            fputtabs(outputFile, innerTabCount);
            fputs("<stringConstant> ", outputFile);
            memmove(line, line+1, strlen(line));
            fputs(line, outputFile);
            fseek(outputFile, -2, SEEK_CUR);
            fputs("</stringConstant>\n", outputFile);
            break;
        case TokenTypeInteger:
            fputtabs(outputFile, innerTabCount);
            fputs("<integerConstant> ", outputFile);
            fputs(line, outputFile);
            fseek(outputFile, -1, SEEK_CUR);
            fputs(" </integerConstant>\n", outputFile);
            break;
        case TokenTypeKeyword:
            fputtabs(outputFile, innerTabCount);
            fputs("<keyword> ", outputFile);
            fputs(line, outputFile);
            fseek(outputFile, -1, SEEK_CUR);
            fputs(" </keyword>\n", outputFile);
            break;
        case TokenTypeIdentifier:
            fputtabs(outputFile, innerTabCount);
            fputs("<identifier> ", outputFile);
            fputs(line, outputFile);
            fseek(outputFile, -1, SEEK_CUR);
            fputs(" </identifier>\n", outputFile);
            
            fpos_t pos;
            fgetpos(inputFile, &pos);
            
            fgets(line, sizeof(line), inputFile);
            if (!strcmp(line, "[\n")) {
                fputtabs(outputFile, innerTabCount);
                fputs("<symbol> [ </symbol>\n", outputFile); //TODO: do this same thing in other places where i can
                
                compileExpression(inputFile, outputFile, innerTabCount);
                
                fgets(line, sizeof(line), inputFile);
                if (!strcmp(line, "]\n")) {
                    fputtabs(outputFile, innerTabCount);
                    fputs("<symbol> ] </symbol>\n", outputFile);
                } else {
                    printf("Expected ']' to end expression!\n");
                    exit(1);
                }
            } else if (!strcmp(line, "(\n")) {
                fputtabs(outputFile, innerTabCount);
                fputs("<symbol> ( </symbol>\n", outputFile);
                
                fpos_t pos;
                fgetpos(inputFile, &pos);
                
                fgets(line, sizeof(line), inputFile);
                if (strcmp(line, ")\n") != 0) {
                    fsetpos(inputFile, &pos);
                    compileExpressionList(inputFile, outputFile, innerTabCount);
                    
                    fgets(line, sizeof(line), inputFile);
                }
                
                if (!strcmp(line, ")\n")) {
                    fputtabs(outputFile, innerTabCount);
                    fputs("<symbol> ) </symbol>\n", outputFile);
                } else {
                    printf("Expected ')' to end expression list!\n");
                    exit(1);
                }
            } else if (!strcmp(line, ".\n")) {
                fputtabs(outputFile, innerTabCount);
                fputs("<symbol> . </symbol>\n", outputFile);
                
                fgets(line, sizeof(line), inputFile);
                if (tokenType(line) == TokenTypeIdentifier) {
                    fputtabs(outputFile, innerTabCount);
                    fputs("<identifier> ", outputFile);
                    fputs(line, outputFile);
                    fseek(outputFile, -1, SEEK_CUR);
                    fputs(" </identifier>\n", outputFile);
                } else {
                    printf("Invalid subroutine name!\n");
                    exit(1);
                }
                
                fgets(line, sizeof(line), inputFile);
                if (!strcmp(line, "(\n")) {
                    fputtabs(outputFile, innerTabCount); //TODO: make this whole chunk a function, because it is used above??
                    fputs("<symbol> ( </symbol>\n", outputFile);
                    
                    fpos_t pos;
                    fgetpos(inputFile, &pos);
                    
                    fgets(line, sizeof(line), inputFile);
                    if (strcmp(line, ")\n") != 0) {
                        fsetpos(inputFile, &pos);
                        compileExpressionList(inputFile, outputFile, innerTabCount);
                        
                        fgets(line, sizeof(line), inputFile);
                    }
                    
                    if (!strcmp(line, ")\n")) {
                        fputtabs(outputFile, innerTabCount);
                        fputs("<symbol> ) </symbol>\n", outputFile);
                    } else {
                        printf("Expected ')' to end expression list!\n");
                        exit(1);
                    }
                } else {
                    printf("Invalid subroutine name!\n");
                    exit(1);
                }
            } else {
                fsetpos(inputFile, &pos);
            }
            break;
        case TokenTypeSymbol:
            if (!strcmp(line, "(\n")) {
                fputtabs(outputFile, innerTabCount);
                fputs("<symbol> ( </symbol>\n", outputFile);
                
                compileExpression(inputFile, outputFile, innerTabCount);
                
                fgets(line, sizeof(line), inputFile);
                if (!strcmp(line, ")\n")) {
                    fputtabs(outputFile, innerTabCount);
                    fputs("<symbol> ) </symbol>\n", outputFile);
                } else {
                    printf("Expected ')' to end expression!\n");
                    exit(1);
                }
            } else if (!strcmp(line, "-\n") || !strcmp(line, "~\n")) {
                fputtabs(outputFile, innerTabCount);
                fputs("<symbol> ", outputFile);
                fputs(line, outputFile);
                fseek(outputFile, -1, SEEK_CUR);
                fputs(" </symbol>\n", outputFile);
                
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
    int outerTabCount = tabCount; //TODO: need a better naming system for the tab counts
    int innerTabCount = tabCount + 1;
    
    fputtabs(outputFile, outerTabCount);
    fputs("<expression>\n", outputFile);
    
    while (1) {
        compileTerm(inputFile, outputFile, innerTabCount);
        
        fpos_t pos;
        fgetpos(inputFile, &pos);
        
        fgets(line, sizeof(line), inputFile);
        if (!strcmp(line, "+\n") || !strcmp(line, "-\n") || !strcmp(line, "*\n") || !strcmp(line, "/\n") || !strcmp(line, "&\n") || !strcmp(line, "|\n") || !strcmp(line, "<\n") || !strcmp(line, ">\n") || !strcmp(line, "=\n")) {
            fputtabs(outputFile, innerTabCount);
            fputs("<symbol> ", outputFile);
            fputs(line, outputFile);
            fseek(outputFile, -1, SEEK_CUR);
            fputs(" </symbol>\n", outputFile);
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
        
        fgets(line, sizeof(line), inputFile);
        if (!strcmp(line, "let\n") || !strcmp(line, "if\n") || !strcmp(line, "while\n") || !strcmp(line, "do\n") || !strcmp(line, "return\n")) {
            char *statementType = malloc(strlen(line));
            strcpy(statementType, line);
            
            fputtabs(outputFile, innerTabCount);
            fputs("<", outputFile);
            fputs(statementType, outputFile);
            fseek(outputFile, -1, SEEK_CUR);
            fputs("Statement>\n", outputFile);
            
            int newInnerTab = innerTabCount + 1;
            
            fputtabs(outputFile, newInnerTab);
            fputs("<keyword> ", outputFile);
            fputs(statementType, outputFile);
            fseek(outputFile, -1, SEEK_CUR);
            fputs(" </keyword>\n", outputFile);
            
            if (!strcmp(line, "let\n")) {
                fgets(line, sizeof(line), inputFile);
                if (tokenType(line) == TokenTypeIdentifier) {
                    fputtabs(outputFile, newInnerTab); //TODO: make functions to print these out??
                    fputs("<identifier> ", outputFile);
                    fputs(line, outputFile);
                    fseek(outputFile, -1, SEEK_CUR);
                    fputs(" </identifier>\n", outputFile);
                } else {
                    printf("Local var name must be of token type 'identifier'!\n");
                    exit(1);
                }
                
                fgets(line, sizeof(line), inputFile);
                if (!strcmp(line, "[\n")) {
                    fputtabs(outputFile, newInnerTab);
                    fputs("<symbol> [ </symbol>\n", outputFile);
                    
                    compileExpression(inputFile, outputFile, newInnerTab);
                    
                    fgets(line, sizeof(line), inputFile);
                    if (!strcmp(line, "]\n")) {
                        fputtabs(outputFile, newInnerTab);
                        fputs("<symbol> ] </symbol>\n", outputFile);
                    } else {
                        printf("Expected ']' to end expression!\n");
                        exit(1);
                    }
                    
                    fgets(line, sizeof(line), inputFile);
                }
                
                if (!strcmp(line, "=\n")) {
                    fputtabs(outputFile, newInnerTab);
                    fputs("<symbol> = </symbol>\n", outputFile);
                } else {
                    printf("Expected '=' after let statement declaration!\n");
                    exit(1);
                }
                
                compileExpression(inputFile, outputFile, newInnerTab);
                
                fgets(line, sizeof(line), inputFile);
                if (!strcmp(line, ";\n")) {
                    fputtabs(outputFile, newInnerTab);
                    fputs("<symbol> ; </symbol>\n", outputFile);
                } else {
                    printf("Expected ';' at end of 'let' statement!\n");
                    exit(1);
                }
            } else if (!strcmp(line, "if\n")) {
                
            } else if (!strcmp(line, "while\n")) {
                
            } else if (!strcmp(line, "do\n")) {
                
            } else if (!strcmp(line, "return\n")) {
                
            }
            
            fputtabs(outputFile, innerTabCount);
            fputs("</", outputFile);
            fputs(statementType, outputFile);
            fseek(outputFile, -1, SEEK_CUR);
            fputs("Statement>\n", outputFile);
            
            free(statementType);
        } else if (!strcmp(line, "}\n")) {
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
    
    fgets(line, sizeof(line), inputFile);
    if (!strcmp(line, "{\n")) {
        fputtabs(outputFile, innerTabCount);
        fputs("<symbol> ", outputFile);
        fputs(line, outputFile);
        fseek(outputFile, -1, SEEK_CUR);
        fputs(" </symbol>\n", outputFile);
    } else {
        printf("Subroutine Body should begin with '{'!\n");
        exit(1);
    }

    while (1) {
        fpos_t pos;
        fgetpos(inputFile, &pos);
        
        fgets(line, sizeof(line), inputFile);
        if (tokenType(line) != TokenTypeKeyword) {
            printf("Only keywords should be specified in class!\n");
            exit(1);
        }
        
        if (!strcmp(line, "var\n")) {
            compileVarDeclaration(inputFile, outputFile, innerTabCount);
        } else if (!strcmp(line, "}\n")) {
            fputtabs(outputFile, innerTabCount);
            fputs("<symbol> ", outputFile);
            fputs(line, outputFile);
            fseek(outputFile, -1, SEEK_CUR);
            fputs(" </symbol>\n", outputFile);
            
            break;
        } else if (!strcmp(line, "let\n") || !strcmp(line, "if\n") || !strcmp(line, "while\n") || !strcmp(line, "do\n") || !strcmp(line, "return\n")) {
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
    
    fputtabs(outputFile, outerTabCount);
    fputs("<subroutineDec>\n", outputFile);
    
    fputtabs(outputFile, innerTabCount);
    fputs("<keyword> ", outputFile);
    fputs(subType, outputFile);
    fseek(outputFile, -1, SEEK_CUR);
    fputs(" </keyword>\n", outputFile);
    
    fgets(line, sizeof(line), inputFile);
    TokenType lineType = tokenType(line);
    if (lineType == TokenTypeIdentifier || !strcmp(line, "int\n") || !strcmp(line, "char\n") || !strcmp(line, "boolean\n") || !strcmp(line, "void\n")) { //TODO: remove dupe
        char *type = malloc(11);
        type = (lineType == TokenTypeIdentifier) ? "identifier" : "keyword";
        
        fputtabs(outputFile, innerTabCount);
        fprintf(outputFile, "<%s> %s", type, line);
        fseek(outputFile, -1, SEEK_CUR);
        fprintf(outputFile, " </%s>\n", type);
    } else {
        printf("Class subroutine declaration does not have a valid return type!\n");
        exit(1);
    }
    
    fgets(line, sizeof(line), inputFile);
    if (tokenType(line) == TokenTypeIdentifier) {
        fputtabs(outputFile, innerTabCount);
        fputs("<identifier> ", outputFile);
        fputs(line, outputFile);
        fseek(outputFile, -1, SEEK_CUR);
        fputs(" </identifier>\n", outputFile);
    } else {
        printf("Class subroutine name must have a valid name!\n");
        exit(1);
    }
    
    fgets(line, sizeof(line), inputFile);
    if (!strcmp(line, "(\n")) {
        fputtabs(outputFile, innerTabCount);
        fputs("<symbol> ", outputFile);
        fputs(line, outputFile);
        fseek(outputFile, -1, SEEK_CUR);
        fputs(" </symbol>\n", outputFile);
    } else {
        printf("Class subroutine missing '('!\n");
        exit(1);
    }
    
    compileParameterList(inputFile, outputFile, innerTabCount); //TODO: do i need a separate function for situations like this? maybe not...a comment could do
    
    fgets(line, sizeof(line), inputFile);
    if (!strcmp(line, ")\n")) {
        fputtabs(outputFile, innerTabCount);
        fputs("<symbol> ", outputFile);
        fputs(line, outputFile);
        fseek(outputFile, -1, SEEK_CUR);
        fputs(" </symbol>\n", outputFile);
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
    
    fgets(line, sizeof(line), inputFile);
    if (!strcmp(line, "class\n")) {
        fputs("<class>\n\t<keyword> class </keyword>\n", outputFile);
    } else {
        printf("File does not begin with a class declaration!\n");
        exit(1);
    }
    
    int tabCount = 1;
    fgets(line, sizeof(line), inputFile);
    if (tokenType(line) == TokenTypeIdentifier) {
        fputtabs(outputFile, tabCount);
        fputs("<identifier> ", outputFile);
        fputs(line, outputFile);
        fseek(outputFile, -1, SEEK_CUR);
        fputs(" </identifier>\n", outputFile);
    } else {
        printf("Class declaration has no class name!\n");
        exit(1);
    }
    
    fgets(line, sizeof(line), inputFile);
    if (tokenType(line) == TokenTypeSymbol) {
        fputtabs(outputFile, tabCount);
        fputs("<symbol> ", outputFile);
        fputs(line, outputFile);
        fseek(outputFile, -1, SEEK_CUR);
        fputs(" </symbol>\n", outputFile);
    } else {
        printf("Class declaration has no class name!\n");
        exit(1);
    }
    
    while (fgets(line, sizeof(line), inputFile)) {
        if (tokenType(line) != TokenTypeKeyword) {
            printf("Only keywords should be specified in class!\n");
            exit(1);
        }
        
        if (!strcmp(line, "field\n") || !strcmp(line, "static\n")) {
            compileClassVarDeclaration(line, inputFile, outputFile, tabCount);
        } else if (!strcmp(line, "constructor\n") || !strcmp(line, "function\n") || !strcmp(line, "method\n")) {
            compileSubroutineDeclaration(line, inputFile, outputFile, tabCount);
        } else {
            printf("Unrecognized keyword specified in class!\n");
            exit(1);
        }
    }
    
    fputs("</class>", outputFile);
}

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
        
        //set up output file for writing
        char *helperPath = malloc(strlen(inputPath) + 1);
        strcpy(helperPath, inputPath);
        char *loc = strrchr(helperPath, '.');
        *(loc) = 0;
        strcat(helperPath, "_helperFile.xml"); //test files already have an xml file, must add "_" to each file to distinguish
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
                
                fpos_t pos;
                fgetpos(helperFile, &pos);
                fseek(helperFile, -1, SEEK_CUR);
                char previous = fgetc(helperFile);
                fsetpos(helperFile, &pos);
                
                if (printingString && c != '"') {
                    fputc(c, helperFile);
                } else if (c == '/' && trimmed[i+1] == '/') { //TODO: check if 'i+1' is out of bounds
                    //do nothing??
                    break;
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
        char *outputPath = malloc(strlen(inputPath) + 1);
        strcpy(outputPath, inputPath);
        char *location = strrchr(outputPath, '.');
        *(location) = 0;
        strcat(outputPath, "_stanOutput.xml"); //test files already have an xml file, must add "_" to each file to distinguish
        FILE *outputFile = fopen(outputPath, "w");
        
        //parser
        rewind(helperFile);
        compileClass(helperFile, outputFile);
        
        //cleanup
        fclose(outputFile);
        
        fclose(helperFile);
        remove(helperPath);
    }
    
    free(files);
    
    return 0;
}
