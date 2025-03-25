#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_BOATS 120
#define MAX_NAME 128
#define MAX_LINE 256

typedef enum { SLIP, LAND, TRAILOR, STORAGE, INVALID } BoatType;

typedef union {
    int slipNumber;
    char bayLetter;
    char trailorTag[16];
    int storageNum;
} LocationInfo;

typedef struct {
    char name[MAX_NAME];
    float length;
    BoatType type;
    LocationInfo location;
    float amountOwed;
} Boat;

Boat *boats[MAX_BOATS];
int boatCount = 0;

const float RATES[] = {12.5, 14.0, 25.0, 11.2};

// ---------- Utility Functions ----------
void toLowerCase(char *str) {
    for (int i = 0; str[i]; i++) str[i] = tolower(str[i]);
}

BoatType parseType(const char *str) {
    if (strcasecmp(str, "slip") == 0) return SLIP;
    if (strcasecmp(str, "land") == 0) return LAND;
    if (strcasecmp(str, "trailor") == 0) return TRAILOR;
    if (strcasecmp(str, "storage") == 0) return STORAGE;
    return INVALID;
}

const char *typeToString(BoatType t) {
    switch (t) {
        case SLIP: return "slip";
        case LAND: return "land";
        case TRAILOR: return "trailor";
        case STORAGE: return "storage";
        default: return "invalid";
    }
}

int nameCompare(const void *a, const void *b) {
    return strcasecmp((*(Boat **)a)->name, (*(Boat **)b)->name);
}

Boat *findBoatByName(char *name) {
    for (int i = 0; i < boatCount; i++) {
        if (strcasecmp(boats[i]->name, name) == 0) return boats[i];
    }
    return NULL;
}

// ---------- CSV I/O ----------
void loadBoats(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) return;

    char line[MAX_LINE];
    while (fgets(line, MAX_LINE, fp)) {
        if (boatCount >= MAX_BOATS) break;

        Boat *b = malloc(sizeof(Boat));
        if (!b) {
            fprintf(stderr, "Memory error\n");
            exit(1);
        }

        char typeStr[20], locStr[20];
        sscanf(line, "%127[^,],%f,%19[^,],%19[^,],%f", b->name, &b->length, typeStr, locStr, &b->amountOwed);
        b->type = parseType(typeStr);

        switch (b->type) {
            case SLIP: b->location.slipNumber = atoi(locStr); break;
            case LAND: b->location.bayLetter = locStr[0]; break;
            case TRAILOR: strncpy(b->location.trailorTag, locStr, 15); break;
            case STORAGE: b->location.storageNum = atoi(locStr); break;
            default: break;
        }

        boats[boatCount++] = b;
    }

    fclose(fp);
    qsort(boats, boatCount, sizeof(Boat *), nameCompare);
}

void saveBoats(const char *filename) {
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        fprintf(stderr, "Error saving file\n");
        return;
    }

    for (int i = 0; i < boatCount; i++) {
        Boat *b = boats[i];
        fprintf(fp, "%s,%.0f,%s,", b->name, b->length, typeToString(b->type));
        switch (b->type) {
            case SLIP: fprintf(fp, "%d", b->location.slipNumber); break;
            case LAND: fprintf(fp, "%c", b->location.bayLetter); break;
            case TRAILOR: fprintf(fp, "%s", b->location.trailorTag); break;
            case STORAGE: fprintf(fp, "%d", b->location.storageNum); break;
            default: break;
        }
        fprintf(fp, ",%.2f\n", b->amountOwed);
    }

    fclose(fp);
}

// ---------- Menu Actions ----------
void printInventory() {
    for (int i = 0; i < boatCount; i++) {
        Boat *b = boats[i];
        printf("%-20s %4.0f' %8s ", b->name, b->length, typeToString(b->type));
        switch (b->type) {
            case SLIP: printf("  # %d", b->location.slipNumber); break;
            case LAND: printf("     %c", b->location.bayLetter); break;
            case TRAILOR: printf("%7s", b->location.trailorTag); break;
            case STORAGE: printf("  # %d", b->location.storageNum); break;
            default: break;
        }
        printf("   Owes $%7.2f\n", b->amountOwed);
    }
}

void addBoat(char *csv) {
    if (boatCount >= MAX_BOATS) {
        printf("Boat limit reached!\n");
        return;
    }

    Boat *b = malloc(sizeof(Boat));
    if (!b) {
        fprintf(stderr, "Malloc failed\n");
        return;
    }

    char typeStr[20], locStr[20];
    sscanf(csv, "%127[^,],%f,%19[^,],%19[^,],%f", b->name, &b->length, typeStr, locStr, &b->amountOwed);
    b->type = parseType(typeStr);

    switch (b->type) {
        case SLIP: b->location.slipNumber = atoi(locStr); break;
        case LAND: b->location.bayLetter = locStr[0]; break;
        case TRAILOR: strncpy(b->location.trailorTag, locStr, 15); break;
        case STORAGE: b->location.storageNum = atoi(locStr); break;
        default: break;
    }

    boats[boatCount++] = b;
    qsort(boats, boatCount, sizeof(Boat *), nameCompare);
}

void removeBoat(char *name) {
    for (int i = 0; i < boatCount; i++) {
        if (strcasecmp(boats[i]->name, name) == 0) {
            free(boats[i]);
            for (int j = i; j < boatCount - 1; j++) {
                boats[j] = boats[j + 1];
            }
            boatCount--;
            return;
        }
    }
    printf("No boat with that name\n");
}

void acceptPayment(char *name, float amount) {
    Boat *b = findBoatByName(name);
    if (!b) {
        printf("No boat with that name\n");
        return;
    }

    if (amount > b->amountOwed) {
        printf("That is more than the amount owed, $%.2f\n", b->amountOwed);
        return;
    }

    b->amountOwed -= amount;
}

void applyMonthlyCharges() {
    for (int i = 0; i < boatCount; i++) {
        Boat *b = boats[i];
        b->amountOwed += b->length * RATES[b->type];
    }
}

// ---------- Main ----------
int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <BoatData.csv>\n", argv[0]);
        return 1;
    }

    loadBoats(argv[1]);
    printf("Welcome to the Boat Management System\n-------------------------------------\n");

    char input[256];
    while (1) {
        printf("\n(I)nventory, (A)dd, (R)emove, (P)ayment, (M)onth, e(X)it : ");
        fgets(input, sizeof(input), stdin);
        char choice = tolower(input[0]);

        if (choice == 'x') {
            printf("\nExiting the Boat Management System\n");
            saveBoats(argv[1]);
            for (int i = 0; i < boatCount; i++) free(boats[i]);
            return 0;
        } else if (choice == 'i') {
            printInventory();
        } else if (choice == 'a') {
            printf("Please enter the boat data in CSV format                 : ");
            fgets(input, sizeof(input), stdin);
            addBoat(input);
        } else if (choice == 'r') {
            printf("Please enter the boat name                               : ");
            fgets(input, sizeof(input), stdin);
            input[strcspn(input, "\n")] = 0;
            removeBoat(input);
        } else if (choice == 'p') {
            printf("Please enter the boat name                               : ");
            fgets(input, sizeof(input), stdin);
            input[strcspn(input, "\n")] = 0;
            char name[128];
            strcpy(name, input);

            printf("Please enter the amount to be paid                       : ");
            fgets(input, sizeof(input), stdin);
            float amount = atof(input);
            acceptPayment(name, amount);
        } else if (choice == 'm') {
            applyMonthlyCharges();
        } else {
            printf("Invalid option %c\n", input[0]);
        }
    }

    return 0;
}
