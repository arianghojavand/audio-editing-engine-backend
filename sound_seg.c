#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>


typedef struct {
    int16_t* data; 
    size_t length; 
    size_t capacity;

} Track;

struct sound_seg {
    
    Track track;
    
    struct sound_seg** track_ptrs[2]; 
    size_t num_track_ptrs; 
    size_t track_ptr_capacity; 



    struct sound_seg* parent;
    struct sound_seg** children;
    size_t num_children;
    size_t num_children_capacity;



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
    struct sound_seg* segment = calloc(1, sizeof(struct sound_seg));
    if (segment == NULL) return NULL;

    segment->track.length = 0;
    segment->track.capacity = 8; //small default size which != 0 so that we can reallocate multiplicatively if needed

    segment->track.data = calloc(segment->track.capacity, sizeof(int16_t));

    if (segment->track.data == NULL) {
        free(segment);
        return NULL;
    }

    // segment->track_ptrs[0] = NULL;
    segment->num_track_ptrs = 0;
    // segment->track_ptr_capacity = 2;
    segment->parent = NULL;
    segment->children = NULL;
    segment->num_children = 0;
    segment->num_children_capacity = 1;
    

    // = malloc(sizeof(struct sound_seg*) * 1);
    // if (segment->children == NULL) {
    //     free(segment->track.data);
    //     free(segment->track_ptrs);
        
    //     free(segment);
    //     return NULL;
    // }

    

    return segment;
}

// Destroy a sound_seg object and free all allocated memory
void tr_destroy(struct sound_seg* obj) {
    if (obj == NULL) return;

    free(obj->track.data);
    // free(obj->track_ptrs);

    for (size_t i = 0; i < obj->num_children; i++) {
        free(obj->children[i]);
    }

    free(obj);

    return;
}

// Return the length of the segment
size_t tr_length(struct sound_seg* seg) {
    if (seg == NULL) return 0;
    return seg->track.length;
}

// Read len elements from position pos into dest
void tr_read(struct sound_seg* track, int16_t* dest, size_t pos, size_t len) {
    if (track == NULL || dest == NULL || pos + len > track->track.length) return;
    memcpy(dest, track->track.data + pos, len * sizeof(int16_t));
}

// Write len elements from src into position pos
void tr_write(struct sound_seg* track, int16_t* src, size_t pos, size_t len) {
    if (track == NULL || src == NULL || len == 0) return;

    size_t required = pos + len;
    if (required > track->track.capacity) {
        size_t new_capacity = track->track.capacity > 0 ? track->track.capacity : 1;
        while (new_capacity < required) {
            new_capacity *= 2;
        }
        int16_t* new_data = realloc(track->track.data, new_capacity * sizeof(int16_t));
        if (new_data == NULL) return;
        track->track.data = new_data;
        track->track.capacity = new_capacity;
    }

    
    if (len > 0) {
        memcpy(track->track.data + pos, src, len * sizeof(int16_t));
    }

    if (required > track->track.length) {
        track->track.length = required;
    }
}

// Delete a range of elements from the track
bool tr_delete_range(struct sound_seg* track, size_t pos, size_t len) {
    if (track == NULL || len > track->track.length || pos > track->track.length - len) return false;

    // Shift samples forward
    memmove(track->track.data + pos, track->track.data + pos + len, 
            (track->track.length - (pos + len)) * sizeof(int16_t));
    
    track->track.length -= len;

    // Optional: Shrink capacity
    if (track->track.length < track->track.capacity / 2) {
        size_t new_capacity = track->track.capacity / 2;
        if (new_capacity < track->track.length) new_capacity = track->track.length; // Don’t undershoot
        int16_t* new_data = realloc(track->track.data, new_capacity * sizeof(int16_t));
        if (new_data == NULL) return false; // Keep old data if fail?
        track->track.data = new_data;
        track->track.capacity = new_capacity;
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

    size_t target_len = target->track.length;
    size_t ad_len = ad->track.length;

    double auto_corr = cross_correlation(ad->track.data, ad->track.data, ad_len);


    size_t buffer_size = 1024; //buffer size for the string
    size_t buffer_offset = 0; //offset for the buffer
    char* result = malloc(buffer_size * sizeof(char)); //result string
    if (result == NULL) return NULL;


    for (size_t i = 0; i <= target_len - ad_len; i++) { //increment by ad_len - 1 to avoid overlap
        double cross_corr = cross_correlation(target->track.data + i, ad->track.data, ad_len);
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


/* used as a helper function for insert
the general idea here is to use a linked list type concept where we treat each soud_seg as 
a node + a pointer to the next node

like kanye we'll chop up the the beat and then stitch it back together

*/

// void copy_children(struct sound_seg* dest, struct sound_seg* src) {
//     if (dest == NULL || src == NULL) return;
//
//     dest->children = malloc(src->num_children * sizeof(struct sound_seg*));
//     if (dest->children == NULL) return;
//
//     for (size_t i = 0; i < src->num_children; i++) {
//         dest->children[i] = src->children[i];
//     }
//
//     dest->num_children = src->num_children;
// }

void link_segments(struct sound_seg* tail_segment, struct sound_seg* head_segment) {
    if (tail_segment == NULL || head_segment == NULL) return;

    // if (tail_segment->num_track_ptrs <= tail_segment->track_ptr_capacity - 1) {
    //     tail_segment->track_ptrs = realloc(tail_segment -> track_ptrs, sizeof(Track*) * (tail_segment->track_ptr_capacity * 2)); 
    //     if (tail_segment->track_ptrs == NULL) return; // Memory allocation failed

    //     tail_segment->track_ptr_capacity *= 2;
    // }
    
    tail_segment->track_ptrs[tail_segment->num_track_ptrs] = &head_segment;
    tail_segment->num_track_ptrs++;

}

void split_track(struct sound_seg* source_track, struct sound_seg* right_track, size_t split_pos) {
    if (source_track == NULL || right_track == NULL) return;

    size_t left_track_len = split_pos;
    size_t right_track_len = source_track->track.length - split_pos;

    // we will simply resize source track for left half and create a new track for the right half

    //right half...
    right_track = tr_init();
    tr_write(right_track, source_track->track.data, split_pos, right_track_len);

    //update parameters and shit for new right half

    right_track->children = source_track->children;
    right_track->num_children = source_track->num_children;
    right_track->num_children_capacity = source_track->num_children_capacity;
    right_track->parent = source_track->parent;

    //resize left half...
    int16_t* temp = realloc(source_track->track.data, left_track_len * sizeof(int16_t));
    if (temp == NULL) {
        // Handle memory allocation failure
        return;
    }

    source_track->track.data = temp;
    source_track->track.length = left_track_len;
    source_track->track.capacity = left_track_len;

    //finally, make sure any pointers from the source track, are now passed on to the right half

    memcpy(right_track->track_ptrs[0], source_track->track_ptrs[0], sizeof(struct sound_seg*));
    memcpy(right_track->track_ptrs[1], source_track->track_ptrs[1], sizeof(struct sound_seg*));
    source_track->num_track_ptrs = 0; //careful: it's not actually 0, but we don't need to worry about it for now

} 



// void copy_parent(struct sound_seg* dest, struct sound_seg* src) {
//     if (dest == NULL || src == NULL) return;

//     dest->parent = src->parent;
// }



// void add_child(struct sound_seg* parent, struct sound_seg* child) {
//     if (parent == NULL || child == NULL) return;

//     if (parent->num_children == parent->num_children_capacity - 1) {
//         parent->num_children_capacity *= 2;
//         parent->children = realloc(parent->children, parent->num_children_capacity * sizeof(struct sound_seg*));
//         if (parent->children == NULL) return; // Memory allocation failed

//     }
//     parent->children[parent->num_children] = child;
//     parent->num_children++;

//     child->parent = parent;
// }





// Insert a portion of src_track into dest_track at position destpos
void tr_insert(struct sound_seg* src_track,
            struct sound_seg* dest_track,
            size_t destpos, size_t srcpos, size_t len) {

    if (src_track == NULL || dest_track == NULL || len == 0) return;

    //split the original source track three ways (like slicing a piece of cake twice to extract a slice)

    struct sound_seg* clip_track = NULL;
    split_track(src_track, clip_track, srcpos);
    link_segments(src_track, clip_track);

    struct sound_seg* rest_of_src = NULL;
    split_track(clip_track, rest_of_src, len);
    link_segments(src_track, rest_of_src); //here the src track (now the left half of the original track) will have two pointers which point to the clip and the right half of the original track

    //we have essentially created three segments out of original source track -> the before clip, the clip, and the after clip

    //now we gotta make a surgical incision into the dest track (where our clip will be inserted)

    struct sound_seg* rest_of_dest = NULL;
    split_track(dest_track, rest_of_dest, destpos);

    link_segments(dest_track, clip_track);
    link_segments(dest_track, rest_of_dest); 

    return;
}



                    


        
    




