#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int fifoQueue[500]; // For FIFO algorithm
int fifoIndex = 0;
long long lruTimestamps[500]; // For LRU algorithm
int lruFrameCount = 0;
int pageTable[1000]; // Page to frame mapping
int frameToPage[500]; // Frame to page mapping

void initFIFO() {
    for (int i = 0; i < 500; i++) fifoQueue[i] = -1;
}

void addToFIFO(int frame) {
    fifoQueue[fifoIndex] = frame;
    fifoIndex = (fifoIndex + 1) % 500;
}

int replacePageFIFO() {
    return fifoQueue[fifoIndex];
}

void initLRU() {
    for (int i = 0; i < 500; i++) lruTimestamps[i] = -1;
}

void updateLRUTimestamp(int frame) {
    lruTimestamps[frame] = ++lruFrameCount;
}

int replacePageLRU() {
    int minFrame = 0;
    long long minTimestamp = lruTimestamps[0];
    for (int i = 1; i < 500; i++) {
        if (lruTimestamps[i] < minTimestamp) {
            minTimestamp = lruTimestamps[i];
            minFrame = i;
        }
    }
    return minFrame;
}

int replacePageRAND(int nframes) {
    return rand() % nframes;
}

void initPageTable(int npages, int nframes) {
    for (int i = 0; i < npages; i++) pageTable[i] = -1;
    for (int i = 0; i < nframes; i++) frameToPage[i] = -1;
}

int countEmptyFrames(int nframes) {
    int count = 0;
    for (int i = 0; i < nframes; i++) {
        if (frameToPage[i] == -1) {
            count++;
        }
    }
    return count;
}

int handlePageFault(int page, int nframes, char *algorithm) {
    int frameToUse = -1;
    for (int i = 0; i < nframes; i++) {
        if (frameToPage[i] == -1) {
            frameToUse = i;
            break;
        }
    }
    if (frameToUse == -1) {
        if (strcmp(algorithm, "rand") == 0) {
            frameToUse = replacePageRAND(nframes);
        } else if (strcmp(algorithm, "fifo") == 0) {
            frameToUse = replacePageFIFO();
        } else if (strcmp(algorithm, "lru") == 0) {
            frameToUse = replacePageLRU();
        }
        int pageToEvict = frameToPage[frameToUse];
        pageTable[pageToEvict] = -1;
    }
    frameToPage[frameToUse] = page;
    pageTable[page] = frameToUse;
    return frameToUse;
}

void generatePageReferences(int *references, int nrefs, int npages, char *locality) {
    srand(time(NULL));
    for (int i = 0; i < nrefs; i++) {
        if (i == 0 || strcmp(locality, "ll") == 0) {
            references[i] = rand() % npages;
        } else {
            int range = (strcmp(locality, "ml") == 0) ? npages * 0.05 : npages * 0.03;
            int lastRef = references[i - 1];
            references[i] = (lastRef + (rand() % (2 * range + 1)) - range) % npages;
            if (references[i] < 0) references[i] += npages;
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 6) {
        fprintf(stderr, "Usage: ./virtmem npages nframes algorithm nrefs locality\n");
        return 1;
    }

    int npages = atoi(argv[1]);
    int nframes = atoi(argv[2]);
    char *algorithm = argv[3];
    int nrefs = atoi(argv[4]);
    char *locality = argv[5];

    int *references = malloc(nrefs * sizeof(int));
    if (!references) {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }

    generatePageReferences(references, nrefs, npages, locality);
    initPageTable(npages, nframes);

    if (strcmp(algorithm, "fifo") == 0) {
        initFIFO();
    } else if (strcmp(algorithm, "lru") == 0) {
        initLRU();
    }

    int pageFaults = 0;
    for (int i = 0; i < nrefs; i++) {
        int currentPage = references[i];
        if (pageTable[currentPage] == -1) {
            handlePageFault(currentPage, nframes, algorithm);
            pageFaults++;
            if (strcmp(algorithm, "fifo") == 0) {
                addToFIFO(currentPage);
            } else if (strcmp(algorithm, "lru") == 0) {
                updateLRUTimestamp(currentPage);
            }
        }
    }

    int emptyFrames = countEmptyFrames(nframes);
    printf("Total number of page faults: %d\n", pageFaults);
    printf("Number of empty frames: %d\n", emptyFrames);

    free(references);
    return 0;
}
