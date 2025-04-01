#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "sound_seg.h"

int main() {
    struct sound_seg* track = tr_init();
    printf("Initial state - Length: %zu, Capacity: %zu\n", tr_length(track), track->capacity); // 0, 8
    
    // Test basic write and read
    int16_t src[15] = {-2,-8,8,-5,3,-2,-9,6,-2,-1,6,-9,7,-4,-7};
    tr_write(track, src, 0, 15);
    printf("After write 15 values - Length: %zu, Capacity: %zu\n", tr_length(track), track->capacity);
    
    int16_t dest[15];
    tr_read(track, dest, 0, 15);
    printf("Read 15 values: ");
    for (int i = 0; i < 15; i++) {
        printf("%d ", dest[i]);
    }
    printf("\n");
    
    // Test delete_range in the middle
    printf("\nTesting tr_delete_range:\n");
    bool success = tr_delete_range(track, 3, 4); // Delete elements 3, 4, 5, 6
    printf("Delete range (3, 4) success: %s\n", success ? "true" : "false");
    printf("After delete - Length: %zu, Capacity: %zu\n", tr_length(track), track->capacity);
    
    // Verify the contents after deletion
    tr_read(track, dest, 0, 6);
    printf("Read after delete: ");
    for (int i = 0; i < 6; i++) {
        printf("%d ", dest[i]);
    }
    printf("\n");

    tr_write(track, ((int16_t[]){ }), 0, 0);  // edge case
    tr_read(track, dest, 0, 0);               // safe but should do nothing
    
    // Test deleting more data to trigger capacity shrink
    success = tr_delete_range(track, 1, 3); // Delete more elements to reduce size
    printf("\nDelete range (1, 3) success: %s\n", success ? "true" : "false");
    printf("After second delete - Length: %zu, Capacity: %zu\n", tr_length(track), track->capacity);
    
    // Verify final contents
    tr_read(track, dest, 0, tr_length(track));
    printf("Final contents: ");
    for (size_t i = 0; i < tr_length(track); i++) {
        printf("%d ", dest[i]);
    }
    printf("\n");
    
    // Test invalid delete
    success = tr_delete_range(track, 10, 2); // Position outside range
    printf("\nInvalid delete test - Success: %s\n", success ? "true" : "false");

    
    // --- WAV TESTING START ---
    printf("\nTesting WAV save and load...\n");

    // Save current track data to file
    wav_save("test_output.wav", track->data, track->length);
    printf("WAV saved to test_output.wav\n");

    // Load it back
    int16_t loaded[100] = {0};
    wav_load("test_output.wav", loaded);

    // Print loaded samples
    printf("WAV loaded samples: ");
    for (size_t i = 0; i < track->length; i++) {
        printf("%d ", loaded[i]);
    }
    printf("\n");
    // --- WAV TESTING END ---

    // === tr_identify TEST ===
    printf("\nTesting tr_identify...\n");

    // Create ad segment with known values
    struct sound_seg* ad = tr_init();
    int16_t ad_samples[] = {42, 43, 44};
    tr_write(ad, ad_samples, 0, 3);

    // Insert ad samples into track at two known positions
    tr_write(track, ad_samples, 1, 3);  // overwrite index 1–3
    tr_write(track, ad_samples, 7, 3);  // overwrite index 7–9

    // Identify ads
    char* result = tr_identify(track, ad);
    printf("Detected ad segments:\n%s", result);

    // Free resources
    free(result);
    tr_destroy(ad);

    
    tr_destroy(track);

     // === EXTRA TEST CASES ===
     printf("\n\n=== EXTRA TEST CASES FOR tr_identify STARTING ===\n");

     // --- Setup: Create a clean track and ad segment ---
     struct sound_seg* t1 = tr_init();
     struct sound_seg* ad1 = tr_init();
 
     int16_t ad_data[] = {100, 200, 300};
     tr_write(ad1, ad_data, 0, 3);
 
     // Track contains 3 clean, non-overlapping ad patterns
     int16_t target_data[] = {
         100, 200, 300,    // match at 0–2
         1, 2, 3,
         100, 200, 300,    // match at 6–8
         7, 8, 9,
         100, 200, 300     // match at 12–14
     };
     tr_write(t1, target_data, 0, sizeof(target_data)/sizeof(int16_t));
 
     // Run identification
     printf("\n[Test Case] tr_identify - multiple non-overlapping patterns...\n");
     char* result1 = tr_identify(t1, ad1);
     printf("Detected:\n%s", result1);
     printf("Expected:\n0,2\n6,8\n12,14\n");
 
     // Free resources
     free(result1);
     tr_destroy(t1);
     tr_destroy(ad1);
 
     // --- Edge Case: No matches ---
     struct sound_seg* t2 = tr_init();
     struct sound_seg* ad2 = tr_init();
 
     int16_t junk[] = {9, 9, 9, 9, 9, 9};
     int16_t fake_ad[] = {1, 2, 3};
 
     tr_write(t2, junk, 0, 6);
     tr_write(ad2, fake_ad, 0, 3);
 
     printf("\n[Test Case] tr_identify - no matches...\n");
     char* result2 = tr_identify(t2, ad2);
     if (result2[0] == '\0') {
         printf("PASS: No matches as expected.\n");
     } else {
         printf("FAIL: Unexpected result = %s\n", result2);
     }
 
     free(result2);
     tr_destroy(t2);
     tr_destroy(ad2);
 
     // --- Edge Case: Single perfect match at end ---
     struct sound_seg* t3 = tr_init();
     struct sound_seg* ad3 = tr_init();
 
     int16_t pattern[] = {4, 5, 6};
     int16_t with_match[] = {0, 0, 0, 4, 5, 6};
 
     tr_write(t3, with_match, 0, 6);
     tr_write(ad3, pattern, 0, 3);
 
     printf("\n[Test Case] tr_identify - match at end...\n");
     char* result3 = tr_identify(t3, ad3);
     printf("Detected: %s", result3);
     printf("Expected: 3,5\n");
 
     free(result3);
     tr_destroy(t3);
     tr_destroy(ad3);
 
     printf("\n=== EXTRA TEST CASES FOR tr_identify COMPLETE ===\n");



    return 0;
}