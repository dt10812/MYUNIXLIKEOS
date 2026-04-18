#include "commands.h"
#include "stdio.h"
#include "string.h"
#include "io.h"

/* Simple password storage - in production, use proper crypto */
static uint32_t system_password_hash = 0;  /* 0 = no password set */

/* Simple hash function */
static uint32_t hash_password(const char* pwd) {
    uint32_t hash = 5381;
    for (int i = 0; pwd[i]; i++) {
        hash = ((hash << 5) + hash) + pwd[i];  /* hash * 33 + c */
    }
    return hash;
}

int cmd_password(int argc, char** argv) {
    (void)argc;
    (void)argv;
    
    char pwd_buf[64];
    char confirm_buf[64];
    
    /* If password is already set, require old password */
    if (system_password_hash != 0) {
        printf("Enter old password: ");
        int i = 0;
        while (i < 63) {
            char c = keyboard_getchar();
            if (c == '\n' || c == '\r') {
                pwd_buf[i] = '\0';
                printf("\n");
                break;
            }
            pwd_buf[i++] = c;
        }
        
        uint32_t entered_hash = hash_password(pwd_buf);
        if (entered_hash != system_password_hash) {
            printf("password: incorrect old password\n");
            return 1;
        }
    }
    
    /* Get new password */
    printf("Enter new password: ");
    int i = 0;
    while (i < 63) {
        char c = keyboard_getchar();
        if (c == '\n' || c == '\r') {
            pwd_buf[i] = '\0';
            printf("\n");
            break;
        }
        pwd_buf[i++] = c;
    }
    
    if (i == 0) {
        printf("password: password too short\n");
        return 1;
    }
    
    /* Confirm new password */
    printf("Confirm new password: ");
    i = 0;
    while (i < 63) {
        char c = keyboard_getchar();
        if (c == '\n' || c == '\r') {
            confirm_buf[i] = '\0';
            printf("\n");
            break;
        }
        confirm_buf[i++] = c;
    }
    
    if (strcmp(pwd_buf, confirm_buf) != 0) {
        printf("password: passwords do not match\n");
        return 1;
    }
    
    system_password_hash = hash_password(pwd_buf);
    printf("password: password changed successfully\n");
    return 0;
}

/* Function to check password (for future login) */
int password_verify(const char* pwd) {
    if (system_password_hash == 0) {
        return 1;  /* No password set, allow access */
    }
    
    uint32_t entered_hash = hash_password(pwd);
    return (entered_hash == system_password_hash) ? 1 : 0;
}
