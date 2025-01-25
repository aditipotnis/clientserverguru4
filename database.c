#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "database.h"

Database db_create(){ 
        Database db;
        db.array = malloc(4 * sizeof(Record));
        db.size = 0;
        db.capacity = 4;
        return db;
}

void db_append(Database * db, Record const * item){
        if (db->size == db->capacity){
                db->capacity *= 2;
                db->array = realloc(db->array, db->capacity * sizeof(Record));
        }
        db->array[db->size] = *item;
        db->size++;
}

Record* db_index(Database * db, int index){
        if (index < 0 || index >= db->size){
                return NULL;
        }
        return &db->array[index];
}

Record* db_lookup(Database * db, char const * handle){
        for (int i = 0; i < db->size; i++){
                if (strcmp(db->array[i].handle, handle) == 0){
                        return &db->array[i];
                }
        }
        return NULL;
}

void db_free(Database * db){
        free(db->array);
        db->array = NULL;
        db->size = 0;
        db->capacity = 0;
}

Record parse_record(char const *line){
	Record record;
	char *line_copy = strdup(line);
	char *token;

	token = strtok(line_copy, ",");
	strncpy(record.handle, token, sizeof(record.handle));
        record.handle[sizeof(record.handle) - 1] = '\0';
	
	token = strtok(NULL, ",");
        record.followerCount = strtoul(token, NULL, 10);

	token = strtok(NULL, ",");
        record.dateLastModified = strtoul(token, NULL, 10);

	token = strtok(NULL, ",");
	strncpy(record.comment, token, sizeof(record.comment));
	record.comment[sizeof(record.comment) - 1] = '\0';

	return record;
}

void db_load_csv(Database *db, char const *path){
        FILE *file = fopen(path, "r");
        if(!file){
                return;
        }

        char *line = NULL;
        size_t len = 0;
        ssize_t read;

        while ((read = getline(&line, &len, file)) != -1){
                if (line[read - 1] == '\n'){
			line[read - 1] = '\0';
			read--;
		}
		
		Record record = parse_record(line);
		db_append(db, &record);
        }

        free(line);
        fclose(file);
}

void db_write_csv(Database *db, char const *path){
        FILE *file = fopen(path, "w");
        if (!file) {
                exit(1);
        }
        for(int i = 0; i < db->size; i++){
		fprintf(file, "%s,%lu,%lu,%s\n", db->array[i].handle, db->array[i].followerCount, db->array[i].dateLastModified, db->array[i].comment);

        }
        fclose(file);
}
