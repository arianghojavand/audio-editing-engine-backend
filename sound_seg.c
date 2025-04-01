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
void wav_load(const char* filename, int16_t* dest) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        perror("Failed to open file");
        return;
    }

    // WAV header is 44 bytes for standard PCM
    fseek(file, 44, SEEK_SET);

    // Read data until end of file
    size_t index = 0;
    while (fread(&dest[index], sizeof(int16_t), 1, file) == 1) {
        index++;
    }

    fclose(file);
}


// Create/write a WAV file from buffer
void wav_save(const char* filename, const int16_t* src, size_t len) {
    FILE* file = fopen(filename, "wb");
    if (!file) {
        perror("Failed to open file");
        return;
    }

    /* 

    WAV file format:
    
    Offset	Size	Field	Description
        0	    4	    "RIFF"	File starts with "RIFF"
        4	    4	    fileSize - 8	Total file size - 8
        8	    4	    "WAVE"	File format ID
        12  	4   	"fmt "	Format chunk ID
        16  	4   	16	Chunk size (for PCM)
        20  	2   	1	Audio format (1 = PCM)
        22  	2   	1	Num channels (1 = mono)
        24  	4   	8000	Sample rate (Hz)
        28  	4   	16000	Byte rate = SampleRate × Ch × BPS/8
        32  	2   	2	Block align = Ch × BPS/8
        34  	2   	16	Bits per sample
        36  	4   	"data"	Data chunk ID
        40  	4   	dataSize	Num samples × Bytes per sample
        44  	.   ..	Audio data	Raw 16-bit PCM samples
        
        */


    uint32_t sample_rate = 8000;
    uint16_t bits_per_sample = 16;
    uint16_t num_channels = 1;

    uint32_t byte_rate = sample_rate * num_channels * (bits_per_sample / 8);
    uint16_t block_align = num_channels * (bits_per_sample / 8);
    uint32_t data_chunk_size = len * sizeof(int16_t);
    uint32_t riff_chunk_size = 36 + data_chunk_size;

    // Write the header
    fwrite("RIFF", 1, 4, file);
    fwrite(&riff_chunk_size, 4, 1, file);
    fwrite("WAVE", 1, 4, file);

    fwrite("fmt ", 1, 4, file);
    uint32_t fmt_chunk_size = 16;
    uint16_t audio_format = 1;  // PCM
    fwrite(&fmt_chunk_size, 4, 1, file);
    fwrite(&audio_format, 2, 1, file);
    fwrite(&num_channels, 2, 1, file);
    fwrite(&sample_rate, 4, 1, file);
    fwrite(&byte_rate, 4, 1, file);
    fwrite(&block_align, 2, 1, file);
    fwrite(&bits_per_sample, 2, 1, file);

    fwrite("data", 1, 4, file);
    fwrite(&data_chunk_size, 4, 1, file);
    fwrite(src, sizeof(int16_t), len, file);

    fclose(file);
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
    if (track == NULL || dest == NULL || pos + len > track->length) return;
    memcpy(dest, track->data + pos, len * sizeof(int16_t));
}

// Write len elements from src into position pos
void tr_write(struct sound_seg* track, int16_t* src, size_t pos, size_t len) {
    if (track == NULL || src == NULL || len == 0) return;

    size_t required = pos + len;
    if (required > track->capacity) {
        size_t new_capacity = track->capacity > 0 ? track->capacity : 1;
        while (new_capacity < required) {
            new_capacity *= 2;
        }
        int16_t* new_data = realloc(track->data, new_capacity * sizeof(int16_t));
        if (new_data == NULL) return;
        track->data = new_data;
        track->capacity = new_capacity;
    }

    
    if (len > 0) {
        memcpy(track->data + pos, src, len * sizeof(int16_t));
    }

    if (required > track->length) {
        track->length = required;
    }
}

// Delete a range of elements from the track
bool tr_delete_range(struct sound_seg* track, size_t pos, size_t len) {
    if (track == NULL || len > track->length || pos > track->length - len) return false;

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



double cross_correlation(const int16_t* data1, const int16_t* data2, size_t len) {
    double sum = 0.0;
    for (size_t n = 0; n < len; n++) {
        sum += (double)data1[n] * (double)data2[n];
    }
    return sum;
}

// Returns a string containing <start>,<end> ad pairs in target
char* tr_identify(struct sound_seg* target, struct sound_seg* ad){
    if (target == NULL || ad == NULL) return strdup("");

    size_t target_len = target->length;
    size_t ad_len = ad->length;

    double auto_corr = cross_correlation(ad->data, ad->data, ad_len);


    size_t buffer_size = 1024; //buffer size for the string
    size_t buffer_offset = 0; //offset for the buffer
    char* result = malloc(buffer_size * sizeof(char)); //result string
    if (result == NULL) return NULL;


    for (size_t i = 0; i <= target_len - ad_len; i++) { //increment by ad_len - 1 to avoid overlap
        double cross_corr = cross_correlation(target->data + i, ad->data, ad_len);
        double similarity = (cross_corr/auto_corr);

        if (similarity >= 0.95) {
            // printf("match found at %zu\n", i);
            // printf("similarity: %lf\n", similarity);
            // printf("cross_corr: %lf\n", cross_corr);
            // printf("auto_corr: %lf\n", auto_corr);

            int chars = snprintf(NULL, 0, "%zu,%zu", i, i + ad_len - 1); //THIS JUST GETS THE FUCKING LENGTH

            if (buffer_offset + chars + 1 >= buffer_size) { // +1 for null terminator - lol
                buffer_size *= 2;
                char* new_result = realloc(result, buffer_size * sizeof(char));
                if (new_result == NULL) {
                    free(result);
                    return NULL;
                }
                result = new_result;
            }

            if (buffer_offset > 0) {
                chars = snprintf(result + buffer_offset, buffer_size - buffer_offset, "\n%zu,%zu", i, i + ad_len - 1);
            } else {
                chars = snprintf(result + buffer_offset, buffer_size - buffer_offset, "%zu,%zu", i, i + ad_len - 1);
            }
            
            buffer_offset += chars;


            i += ad_len > 1 ? ad_len - 1 : ad_len; //increment by ad_len - 1 to avoid overlap

        }

        

    }

    if (buffer_offset == 0) {
        return strdup("");
    }

    return result;
    
}

// Insert a portion of src_track into dest_track at position destpos
void tr_insert(struct sound_seg* src_track,
            struct sound_seg* dest_track,
            size_t destpos, size_t srcpos, size_t len) {
    return;
}




