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
        char outputPath[strlen(inputPath) + 1];
        strcpy(outputPath, inputPath);
        char *loc = strrchr(outputPath, '.');
        *(loc) = 0;
        strcat(outputPath, "_stanOutput.xml"); //test files already have an xml file, must add '_test' to each file to distinguish
        FILE *outputFile = fopen(outputPath, "w+");
        
        free(inputPath);
        
        char line[256];
        while (fgets(line, sizeof(line), inputFile)) {
            char *trimmed = trim_whitespace(line);
            
            if ((trimmed[0] == '/' && (trimmed[1] == '/' || trimmed[1] == '*')) || trimmed[0] == '*' ||
                isspace(trimmed[0]) || strcmp(trimmed, "") == 0 || strcmp(trimmed, "\r\n") == 0) { continue; }
            
            int printingString = 0;
            for (int i = 0; i < strlen(trimmed); i++) {
                char c = trimmed[i];
                
                fpos_t pos;
                fgetpos(outputFile, &pos);
                fseek(outputFile, -1, SEEK_END);
                int previous = fgetc(outputFile);
                fsetpos(outputFile, &pos);
                
                if (printingString && c != '"') {
                    fputc(c, outputFile);
                } else if (c == '{' || c == '}' || c == '(' || c == ')' || c == '[' || c == ']' || c == '.' ||
                           c == ',' || c == ';' || c == '+' || c == '-' || c == '*' || c == '/' || c == '&' ||
                           c == '|' || c == '<' || c == '>' || c == '=' || c == '~') {
                    if (previous != '\n') {
                        fputc('\n', outputFile);
                    }
                    
                    if (c == '<') {
                        fputs("&lt;", outputFile);
                    } else if (c == '>') {
                        fputs("&gt;", outputFile);
                    } else if (c == '&') {
                        fputs("&amp;", outputFile);
                    } else {
                        fputc(c, outputFile);
                    }
                    
                    fputc('\n', outputFile);
                } else if (c == '"') {
                    if (previous != '\n' && !printingString) {
                        fputc('\n', outputFile);
                    }
                    
                    printingString = !printingString;
                } else if (c == ' ') {
                    if (previous != '\n') {
                        fputc('\n', outputFile);
                    }
                } else {
                    fputc(c, outputFile);
                }
            }
        }
        
        fclose(outputFile);
    }
    
    free(files);
    
    return 0;
}
