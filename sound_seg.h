// sound_seg.h
#ifndef SOUND_SEG_H
#define SOUND_SEG_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

struct sound_seg {
    int16_t* data;
    size_t length;
    size_t capacity;
};

struct sound_seg* tr_init();
void wav_load(const char* filename, int16_t* dest);
void wav_save(const char* filename, const int16_t* src, size_t len);
void tr_destroy(struct sound_seg* obj);
size_t tr_length(struct sound_seg* seg);
void tr_read(struct sound_seg* track, int16_t* dest, size_t pos, size_t len);
void tr_write(struct sound_seg* track, int16_t* src, size_t pos, size_t len);
bool tr_delete_range(struct sound_seg* track, size_t pos, size_t len);
void tr_insert(struct sound_seg* dest, struct sound_seg* src, size_t destpos, size_t srcpos, size_t len);
double cross_correlation(const int16_t* data1, const int16_t* data2, size_t len);
char* tr_identify(struct sound_seg* target, struct sound_seg* ad);

#endif