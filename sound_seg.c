#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>


struct sound_seg {
    //TODO
    int16_t* data; 
    size_t length; 
    size_t capacity; 

};

// Load a WAV file into buffer
void wav_load(const char* filename, int16_t* dest){
    


    return;
}

// Create/write a WAV file from buffer
void wav_save(const char* fname, int16_t* src, size_t len){
    return;
}

// Initialize a new sound_seg object
struct sound_seg* tr_init() {
    struct sound_seg* track = calloc(1, sizeof(struct sound_seg));
    if (track == NULL) return NULL;

    track->length = 0;
    track->capacity = 8; //small default size which != 0 so that we can reallocate multiplicatively if needed

    track->data = calloc(track->capacity, sizeof(int16_t));

    if (track->data == NULL) {
        free(track);
        return NULL;
    }

    return track;
}

// Destroy a sound_seg object and free all allocated memory
void tr_destroy(struct sound_seg* obj) {
    if (obj == NULL) return;

    free(obj->data);
    free(obj);

    return;
}

// Return the length of the segment
size_t tr_length(struct sound_seg* seg) {
    if (seg == NULL) return 0;
    return seg->length;
}

// Read len elements from position pos into dest
void tr_read(struct sound_seg* track, int16_t* dest, size_t pos, size_t len) {
    if (track == NULL || dest == NULL || pos > track->length) return;
    memcpy(dest, track->data + pos, len * sizeof(int16_t));
}

// Write len elements from src into position pos
void tr_write(struct sound_seg* track, int16_t* src, size_t pos, size_t len) {
    if (track == NULL || src == NULL || len == 0) return;

    if (pos + len > track->capacity) {
        size_t new_capacity = track->capacity * 2;
        while (new_capacity < pos + len) {
            new_capacity *= 2;
        }
        int16_t* new_data = realloc(track->data, new_capacity * sizeof(int16_t));
        if (new_data == NULL) return;
        track->data = new_data;
        track->capacity = new_capacity;
    }

    if (pos + len > track->length) {
        track->length = pos + len;
    }

    memcpy(track->data + pos, src, len * sizeof(int16_t));

    return;
}

// Delete a range of elements from the track
bool tr_delete_range(struct sound_seg* track, size_t pos, size_t len) {
    if (track == NULL || pos + len > track->length) return false;

    // Shift samples forward
    memmove(track->data + pos, track->data + pos + len, 
            (track->length - (pos + len)) * sizeof(int16_t));
    
    track->length -= len;

    // Optional: Shrink capacity
    if (track->length < track->capacity / 2) {
        size_t new_capacity = track->capacity / 2;
        if (new_capacity < track->length) new_capacity = track->length; // Don’t undershoot
        int16_t* new_data = realloc(track->data, new_capacity * sizeof(int16_t));
        if (new_data == NULL) return false; // Keep old data if fail?
        track->data = new_data;
        track->capacity = new_capacity;
    }

    return true;
}

// Returns a string containing <start>,<end> ad pairs in target
char* tr_identify(struct sound_seg* target, struct sound_seg* ad){
    return NULL;
}

// Insert a portion of src_track into dest_track at position destpos
void tr_insert(struct sound_seg* src_track,
            struct sound_seg* dest_track,
            size_t destpos, size_t srcpos, size_t len) {
    return;
}
