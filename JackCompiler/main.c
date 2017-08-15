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

int is_jack_file(const char *file) {
    const char *dot = strrchr(file, '.');
    if(!dot || dot == file) { return 0; }
    
    const char *extension = dot + 1;
    return (strcmp(extension, "jack") == 0) ? 1 : 0;
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
        printf("No virtual machine files found\n");
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
        
        char line[256];
        while (fgets(line, sizeof(line), inputFile)) {
            char *trimmed = trim_whitespace(line);
            
            if ((trimmed[0] == '/' && (trimmed[1] == '/' || trimmed[1] == '*')) || trimmed[0] == '*' ||
                isspace(trimmed[0]) || strcmp(trimmed, "") == 0 || strcmp(trimmed, "\r\n") == 0) { continue; }
            
            int printingString = 0;
            for (int i = 0; i < strlen(trimmed); i++) {
                char c = trimmed[i];
                
                fpos_t pos;
                fgetpos(helperFile, &pos);
                fseek(helperFile, -1, SEEK_END);
                int previous = fgetc(helperFile);
                fsetpos(helperFile, &pos);
                
                if (printingString && c != '"') {
                    fputc(c, helperFile);
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
                    if (printingString && previous != '\n') {
                        fputc('\n', helperFile);
                    }
                    
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
        
        fputs("<tokens>\n", outputFile);
        
        rewind(helperFile);
        while (fgets(line, sizeof(line), helperFile)) {
            if (isSymbol(line[0])) {
                fputs("<symbol> ", outputFile);
                fputs(line, outputFile);
                fseek(outputFile, -1, SEEK_CUR);
                fputs(" </symbol>\n", outputFile);
            } else {
                fputs(line, outputFile);
            }
        }
        
        fputs("</tokens>", outputFile);
        fclose(outputFile);
        
        fclose(helperFile);
        remove(helperPath);
    }
    
    free(files);
    
    return 0;
}
