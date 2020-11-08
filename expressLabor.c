#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>     //open,creat
#include <sys/types.h> //open
#include <sys/stat.h>
#include <errno.h> //perror, errno
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

// const char *dayNames[] = {"hétfő", "kedd", "szerda", "csütörtök", "péntek", "szombat", "vasárnap"};
const char *testTypes[] = { "Megfazas", "COVID-19", "Tudogyulladas", "Kiutes" }; 
#define SAVE_FILE_NAME "vinery.txt"

int FindTest(const char *testType)
{
    for (int i = 0; i < sizeof(testTypes) / sizeof(testTypes[0]); ++i)
    {
        if (strcasecmp(testTypes[i], testType) == 0) //not case sensitive
            return i;
    }
    fprintf(stderr, "Nem adtak meg %s tipusu tesztet.\n", testType);
    return -1;
}

// Trim whitespaces
char* TrimWS(char* s) {
    while(isspace(*(unsigned char*)s)) s++;
    char* last = s + strlen(s);
    while(last!=s) {
        --last;
        if (!isspace(*(unsigned char*)last))
            break;
        *last = 0;
    }
    // printf("Név: %s\n",s);
    return s;
}

char* ReadTrimmed(FILE* f, char* buf, int bufLen) {
    char* s = fgets(buf, bufLen, f);
    return TrimWS(s);
}

char* ReadTrimmedInput(const char* msg, char* buf, int bufLen) {
    printf("%s\n", msg);
    return ReadTrimmed(stdin, buf, bufLen);
}

typedef struct Record_
{
    char *name;
    char *addr;
    char* taj;
    int testMask;
    int express; // 1 true | 0 false
} Record;

// typedef struct DayData_
// {
//     int numReq;
//     int numVol;
// } DayData;

// DayData dd[7] = {{0, 0}};
Record *records = NULL;
int numRecords = 0;
int allocatedRecords = 0;

void ModifyData(int mask, int inc)
{
    int idx = 0;
    while (mask)
    {
        // if (mask & 1)
        // {
        //     dd[idx].numVol += inc; // nem lesz negativ <= ORSI (inv.)
        // }
        mask >>= 1; // mask = mask >> 1;
        ++idx;
    }
}
void AddDayData(int mask)
{
    ModifyData(mask, 1);
}

void RemoveDayData(int mask)
{
    ModifyData(mask, -1);
}

// bool CheckDayCapacity(int mask, int inc ) {
//     int idx = 0;
//     int capMask = 0;
//     while (mask)
//     {
//         if ((mask & 1) && (dd[idx].numVol + inc < dd[idx].numReq))
//         {
//             capMask |= 1U<<idx; // |= - bitwise or ; 1U - unsigned int
//         }
//         mask >>= 1; // mask = mask >> 1;
//         ++idx;
//     }
// }

Record *NewRecord()
{
    if (numRecords >= allocatedRecords)
    {
        allocatedRecords += allocatedRecords / 2 + 1;
        records = realloc(records, allocatedRecords * sizeof(Record));
    }
    return &records[numRecords++];
}

Record* AddNewRecord(const char* name, const char* addr, const char* taj, int testMask, int express) {
    Record* rec = NewRecord();
    rec->name = strdup(name);
    rec->addr = strdup(addr);
    rec->taj = strdup(taj);
    rec->testMask = testMask;
    rec->express = express;
    return rec;
}

void WriteSingleRecord(FILE *f, const Record *rec)
{
    fprintf( f, "%s\n%s\n%s\n%d\n%d\n", rec->name, rec->addr, rec->taj, rec->testMask, rec->express );
}

void SaveAllRecords()
{
    FILE *f = fopen(SAVE_FILE_NAME, "w");
    if (!f)
    {
        perror("Nem lehet menteni a " SAVE_FILE_NAME " fileba..\n");
        return;
    }
    // for (int i = 0; i < 7; ++i)
    // {
    //     fprintf(f, "%d ", dd[i].numReq);
    // }
    for (int i = 0; i < numRecords; ++i)
    {
        WriteSingleRecord(f, &records[i]);
    }
    fputs("\n", f);
    fclose(f);
}

void AppendRecordToFile(const Record *rec)
{
    FILE *f = fopen(SAVE_FILE_NAME, "a");
    if (!f)
    {
        perror("Nem lehet menteni a " SAVE_FILE_NAME " fileba.\n");
        return;
    }
    WriteSingleRecord(f, rec);
    fclose(f);
}

// void QueryAndSaveReqWorkers()
// {
//     printf("Még nincsenek meg a borászat igényei!\n");
//     for (int i = 0; i < 7; i++)
//     {
//         printf("Adja meg, hogy a %s napra mennyi munkás szükséges: ", testTypes[i]);
//         scanf("%d", &dd[i].numReq);
//     }
//     SaveAllRecords();
// }

bool InitRecords()
{
    // memset(dd, 0, sizeof(dd));

    FILE *f = fopen(SAVE_FILE_NAME, "r");
    if (!f)
        return false;

    // for (int i = 0; i < 7; ++i)
    // {
    //     if (fscanf(f, "%d", &dd[i].numReq) < 0)
    //     {
    //         fclose(f);
    //         return false;
    //     }
    // }
    char line[200];
    while(true) {
        char* r= fgets(line, sizeof(line), f);
        if ( !r) break; // probably EOF
        r= fgets(line, sizeof(line), f);
        if ( !r) break; // probably EOF
        Record* nr = NewRecord();
        nr->name = strdup(TrimWS(line));
        nr->addr = strdup(ReadTrimmed(f, line, sizeof(line)));
        fscanf(f, "%d", &nr->testMask);
        // printf("addday\n");
        AddDayData(nr->testMask);    
    }
    fclose(f);
    return true;
}

void FreeRecordData(Record *rec)
{
    free(rec->addr);
    free(rec->name);
}

void CleanupRecords()
{
    for (int i = 0; i < numRecords; ++i)
    {
        FreeRecordData(records + i);
    }
}

void EraseRecordAndSave(Record *rec)
{
    RemoveDayData(rec->testMask);
    FreeRecordData(rec);
    Record *last = records + numRecords;
    if (rec + 1 != last)
        memmove(rec, rec + 1, (last - rec - 1) * sizeof(*rec));
    --numRecords;
    SaveAllRecords();
}
void EraseRecordByIdxAndSave(int idx)
{
    if (idx < 0 || idx >= numRecords)
        return;
    EraseRecordAndSave(records + idx);
}

int ReadDays(char* msg, bool skipCapacityCheck)
{
    char days[55];
    while(true) {
        char* trimmed = ReadTrimmedInput(msg, days, sizeof(days));
        if (!trimmed || !trimmed[0])
            break;

        char *strtokstate = NULL;
        bool badInput = false;
        int dayMask = 0;
        for (char *s = strtok_r(days, " \t\n\r", &strtokstate); s; s = strtok_r(NULL, " \t\n\r", &strtokstate))
        {
            int d = FindTest(s);
            if (d >= 0)
                dayMask |= (1 << d);
            else
                badInput = true;
        }
        if (badInput)
            continue;
        if (
        skipCapacityCheck)
            return dayMask;
        // int okMask = CheckDayCapacity(dayMask, 1);
        // if (okMask == dayMask)
        //     return dayMask;
        // printf("Minden hely foglalt az adott napon (%d)\n", dayMask & ~okMask); // should not be only a number
    }
    return -1;
}

void ListATest(int testIdx) 
{
    int mask = 1 << testIdx;
    printf("%s /*(%d/%d):*/\n", testTypes[ testIdx ]/*, dd[ testIdx ].numVol, dd[ testIdx ].numReq*/ );
    for(int i = 0; i<numRecords; ++i)
    {
        const Record* r = records + i;
        if (r->testMask & mask)
            printf("\t%s(%s)\n", r->name, r->addr);
    }
}

void ListAllTests()
{
    for(int d = 0; d<7; ++d)
    {
        ListATest(d);
        printf("\n");
    }
}

// finds the saved order identified by the costumers' name
void QueryAndListATest()
{
    char testName[55];
    char* trimmed = ReadTrimmedInput("Adja meg a kivant tesztet", testName, sizeof(testName));
    if (!trimmed || !trimmed[0])
        return;
    int d = FindTest(trimmed);
    if (d >= 0)
        ListATest(d);
    printf("\n");
}

Record* QueryAndFindRecord()
{
    char _name[55];
    char* name = ReadTrimmedInput("Adja meg a nevet", _name, sizeof(_name));
    if (!name || !name[0])
        return NULL;
    for(int i=0; i<numRecords; ++i) {
        if (strcasecmp(records[i].name, name) == 0)
            return records + i;
    }
    printf("Nem talaltam ilyen nevu egyent!\n");
    return NULL;
}

int main(int argc, char **argv)
{
    // if (!InitRecords())
    //     QueryAndSaveReqWorkers();
    // input: what do you want to do
    int isRunning = 1; // 1 true | 0 false
    while (isRunning == 1)
    {
        printf("Valasszon az alabbi lehetosegek kozul!\n");
        printf("1 Uj adat bevitele\n");
        printf("2 Meglevo adat modositasa\n");
        printf("3 Meglevo adat torlese\n");
        printf("4 Lista keszitese teszttipusrol\n");
        printf("5 Teljes lista keszitese\n");

        int resp;
        int res = scanf("%d", &resp);
        if (res == EOF)
            break;
        char dummy[20];
        fgets(dummy, sizeof(dummy), stdin);
        switch (resp)
        {
        // new order
        case 1: {
            char name[50];
            char address[100];
            char TAJ[12];
            char express[5]; 

            ReadTrimmedInput("Adja meg a nevet", name, sizeof(name));
            ReadTrimmedInput("Adja meg a cimet", address, sizeof(address));
            ReadTrimmedInput("Adja meg a TAJ szamat", TAJ, sizeof(TAJ));
            int testMask = ReadDays("Adja meg, hogy milyen tipusu tesztet / teszteket ker (szóközökkel elválasztva)", false);
            if (testMask < 0) {
                printf("nincsen teszttipus megadva\n"); // no record
                continue;
            }
            ReadTrimmedInput( "Kivanja igenybe venni a hatosagi arnal dragabb, de gyorsabb EXPRESS szolgaltatast? [ igen/nem ]", express, sizeof( express ) );
            int exp;
            if ( express == "igen" ) exp = 1;
            else exp = 0;
            Record *orderData = AddNewRecord( name, address, TAJ, testMask, exp );
            AddDayData( testMask );
            AppendRecordToFile( orderData );
            break; }
        // Modify one order
        case 2: {
            Record* rec = QueryAndFindRecord();
            if (!rec) break; // QueryAndFindRecord prints if the name was not found
            while(true) {
                int testMask = ReadDays("Adja meg, hogy milyen tipusu teszteket ker (szokozokkel elvalasztva)", true);
                if (testMask >= 0 && testMask != rec->testMask)
                {
                    int nd = testMask & ~rec->testMask;
                    // if (CheckDayCapacity(nd,1) != nd)
                    // {
                    //     printf("Egyik nap mar tele van, uj adatokat kerunk\n");
                    //     continue;
                    // }
                    RemoveDayData(rec->testMask);
                    rec->testMask = testMask;
                    AddDayData(rec->testMask);
                }
                else if (testMask < 0)
                    printf("nincsenek tesztek megadva\n"); // no record
                else
                    printf("Nem modosult adat");
                break;             
            }

            break; }
        // Delete one order
        case 3: {
            Record* rec = QueryAndFindRecord();
            if (rec)
                EraseRecordAndSave(rec);
            break; }
        // List one type of test
        case 4: 
            QueryAndListATest();
            break; 
        // List all test orders  
        case 5: 
            ListAllTests();
            break;
        // // Napi telitettség - regi
        // case 6:
        //     for(int i=0; i < 7; ++i) {
        //         printf("%d/%d ", dd[i].numVol, dd[i].numReq);
        //     }
        //     printf("\n");
        //     break;
        // Close everything
        case 7:
            isRunning = 0;
            break;
        // Input is not a valid choice
        default:
            continue;
        }
    }
    CleanupRecords(); // not necessary before exit but be nice
}