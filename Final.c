/*
============================================================================
Name            : EECS 348 Assignment 2
Author          : Kaden Shepherd
Description     : C program that implements a priority queue for managing emails.
Inputs          : Test file containing EMAIL, NEXT, READ, and COUNT commands.
Output          : Displays the next email, counts of emails, and handles reading emails based on priority.
Collaborators   : None
Other Sources   : Two models for ai generation, as well as a 3rd model used for general assistance as visible in log
Creation Date   : September 17, 2026
Revisions       : Added invalid-input handling and concise explanatory comments.
============================================================================
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Structure to represent an email
typedef struct {
    char sender[32];
    char subject[256];
    char date[11]; // MM-DD-YYYY
    int priority;
    int date_value;
} Email;

// Structure for the Priority Queue (MaxHeap using dynamic list)
typedef struct {
    Email *data;
    int capacity;
    int size;
} MaxHeap;

// Helper: Convert MM-DD-YYYY to YYYYMMDD and reject invalid dates.
int parse_date(const char *date_str, int *date_value) {
    int month;
    int day;
    int year;
    char extra;
    int days_in_month;

    if (sscanf(date_str, "%d-%d-%d%c", &month, &day, &year, &extra) != 3 ||
        year < 0 || month < 1 || month > 12) {
        return 0;
    }

    days_in_month = 31;
    if (month == 4 || month == 6 || month == 9 || month == 11) {
        days_in_month = 30;
    } else if (month == 2) {
        int leap_year = (year % 400 == 0) ||
                        (year % 4 == 0 && year % 100 != 0);
        days_in_month = leap_year ? 29 : 28;
    }

    if (day < 1 || day > days_in_month) {
        return 0;
    }

    *date_value = year * 10000 + month * 100 + day;
    return 1;
}

// Helper: Convert sender category to base priority level (higher number = higher priority)
int get_sender_priority(const char *sender) {
    if (strcmp(sender, "Boss") == 0) return 5;
    if (strcmp(sender, "Subordinate") == 0) return 4;
    if (strcmp(sender, "Peer") == 0) return 3;
    if (strcmp(sender, "ImportantPerson") == 0) return 2;
    if (strcmp(sender, "OtherPerson") == 0) return 1;
    return 0;
}

// Compare two emails: Returns > 0 if email 'a' has higher priority than 'b'
int compare_emails(const Email *a, const Email *b) {
    // Priority 1: Sender Category
    if (a->priority != b->priority) {
        return a->priority - b->priority;
    }
    
    // Priority 2: Newest email first (Tie-breaker using Date)
    return a->date_value - b->date_value;
}

// Initialize MaxHeap
MaxHeap* create_heap(int initial_capacity) {
    MaxHeap *heap = malloc(sizeof(MaxHeap));
    if (heap == NULL) {
        return NULL;
    }

    heap->capacity = initial_capacity;
    heap->size = 0;
    heap->data = malloc(sizeof(Email) * heap->capacity);
    if (heap->data == NULL) {
        free(heap);
        return NULL;
    }

    return heap;
}

// Swap helper
void swap(Email *a, Email *b) {
    Email temp = *a;
    *a = *b;
    *b = temp;
}

// Heapify Up (used during insertion)
void heapify_up(MaxHeap *heap, int index) {
    while (index > 0) {
        int parent = (index - 1) / 2;
        if (compare_emails(&heap->data[index], &heap->data[parent]) > 0) {
            swap(&heap->data[index], &heap->data[parent]);
            index = parent;
        } else {
            break;
        }
    }
}

// Heapify Down (used during extraction)
void heapify_down(MaxHeap *heap, int index) {
    int max_idx = index;
    while (1) {
        int left = 2 * index + 1;
        int right = 2 * index + 2;

        if (left < heap->size && compare_emails(&heap->data[left], &heap->data[max_idx]) > 0) {
            max_idx = left;
        }
        if (right < heap->size && compare_emails(&heap->data[right], &heap->data[max_idx]) > 0) {
            max_idx = right;
        }

        if (max_idx != index) {
            swap(&heap->data[index], &heap->data[max_idx]);
            index = max_idx;
        } else {
            break;
        }
    }
}

// Push an email to MaxHeap
void heap_push(MaxHeap *heap, Email email) {
    if (heap->size == heap->capacity) {
        int new_capacity = heap->capacity * 2;
        Email *new_data = realloc(heap->data,
                                  sizeof(Email) * new_capacity);
        if (new_data == NULL) {
            return;
        }
        heap->data = new_data;
        heap->capacity = new_capacity;
    }
    heap->data[heap->size] = email;
    heap->size++;
    heapify_up(heap, heap->size - 1);
}

// Pop (Remove and return) the highest-priority email
int heap_pop(MaxHeap *heap, Email *output) {
    if (heap->size == 0) return 0; // Empty
    *output = heap->data[0];
    heap->data[0] = heap->data[heap->size - 1];
    heap->size--;
    heapify_down(heap, 0);
    return 1;
}

// Peek at highest-priority email without removing
int heap_peek(MaxHeap *heap, Email *output) {
    if (heap->size == 0) return 0; // Empty
    *output = heap->data[0];
    return 1;
}

// Free allocated memory
void free_heap(MaxHeap *heap) {
    free(heap->data);
    free(heap);
}

int main(int argc, char *argv[]) {
    MaxHeap *inbox;
    char line[512];
    FILE *input_file;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <test_file>\n", argv[0]);
        return 1;
    }

    input_file = fopen(argv[1], "r");
    if (input_file == NULL) {
        fprintf(stderr, "Unable to open test file: %s\n", argv[1]);
        return 1;
    }

    inbox = create_heap(10);

    if (inbox == NULL) {
        fprintf(stderr, "Unable to allocate the email queue.\n");
        fclose(input_file);
        return 1;
    }

    while (fgets(line, sizeof(line), input_file)) {
        // Strip trailing newline/carriage return
        line[strcspn(line, "\r\n")] = 0;
        if (strlen(line) == 0) continue;

        if (strncmp(line, "EMAIL ", 6) == 0) {
            Email e = {0};
            char *payload = line + 6;
            int date_value;
            
            // Parse comma-delimited fields: <sender>,<subject>,<date>
            char *sender = strtok(payload, ",");
            char *subject = strtok(NULL, ",");
            char *date = strtok(NULL, ",");

            if (sender && subject && date) {
                while (isspace((unsigned char)*sender)) sender++;
                while (isspace((unsigned char)*subject)) subject++;
                while (isspace((unsigned char)*date)) date++;

                if (get_sender_priority(sender) == 0 ||
                    strlen(sender) >= sizeof(e.sender) ||
                    strlen(subject) >= sizeof(e.subject) ||
                    strlen(date) >= sizeof(e.date) ||
                    !parse_date(date, &date_value)) {
                    continue;
                }

                strncpy(e.sender, sender, sizeof(e.sender) - 1);
                strncpy(e.subject, subject, sizeof(e.subject) - 1);
                strncpy(e.date, date, sizeof(e.date) - 1);
                e.priority = get_sender_priority(e.sender);
                e.date_value = date_value;

                heap_push(inbox, e);
            }
        } 
        else if (strcmp(line, "COUNT") == 0) {
            printf("There are %d emails to read.\n", inbox->size);
        } 
        else if (strcmp(line, "NEXT") == 0) {
            Email top;
            if (heap_peek(inbox, &top)) {
                printf("Next email:\n");
                printf("Sender: %s\n", top.sender);
                printf("Subject: %s\n", top.subject);
                printf("Date: %s\n", top.date);
            }
        } 
        else if (strcmp(line, "READ") == 0) {
            Email read_email;
            heap_pop(inbox, &read_email);
        }
    }

    free_heap(inbox);
    fclose(input_file);
    return 0;
}