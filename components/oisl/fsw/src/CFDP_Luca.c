/*******************************************************
 * Core O-ISL script to exchange data between two satellites.
** To adapt it to this satellite modify Sat_Name.
** For large file transfer and routing modify also:
** my_src:    filename containing the data (text) to transfer from this satellite to the receiver
** for_dest:  directory where the file must be moved if the receiver is the forward satellite
** back_dest: directory where the file must be moved if the receiver is the backward satellite
** For large file transfer modify transfer_time_to_add and OGS_ASSUMED
 ********************************************************/
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "CFDP_PDU.h"
#include "oisl_app.h"
#include "CFDP_Luca.h"
#include <time.h>
#include <math.h>

#include "generic_adcs_msgids.h"
#include "generic_adcs_msg.h"
#include "generic_adcs_adac.h"
#include "cfe.h"

/************************************************************
 *                      Satellite and Link properties       *
 ************************************************************/

static const char* Sat_Name = "Sat_1_1";
const size_t memoryCapacity = 8e9;                                   // 8 GB for payload data
const double transferSpeedMbps = 100.0;                              // Transfer speed in Mbps
const double TransferCapacityperSecond = (transferSpeedMbps/8)*1e6;   // 100Mbps = 12.5e6 Bytes per second

const int segmentSize = CF_MAX_PDU_SIZE - sizeof(CF_CFDP_PduFileDataHeader_t) - CF_CFDP_MIN_HEADER_SIZE;

#define MAX_CANDIDATES 24  // Maximum number of satellites in constellation/orbital ring

// Global memory status for receiver
MemoryStatus receiverMemory = {
    .totalSize = 8e9,  // Bytes
    .currentUsed = 0,
    .isAvailable = true
};

MemoryStatus memoryStatusInstance = {
    .totalSize = 8e9,  // Bytes
    .currentUsed = 0,
    .isAvailable = true
};

MemoryStatus *memoryInfo = &memoryStatusInstance;

uint8_t transfering = 0;
uint8_t *transferActive = &transfering;

/************************************************************
 *                      File paths                          *
 ************************************************************/

static const char* fileSent_confirmation = "/mnt/extras/SSD/NOS3_RBT/nos3_local_OISL/file_sent.txt";

static const char* my_src = "/mnt/extras/SSD/NOS3_RBT/nos3_luca_OISL/nos3_rbt/components/oisl/fsw/src/files_Test/plainText.txt";
static const char* for_dest = "/mnt/extras/SSD/NOS3_RBT/nos3_luca_OISL/forward_sat/nos3_rbt/COSMOS_Control/Execution/OISL/files_received/plainText.txt";
static const char* back_dest = "/mnt/extras/SSD/NOS3_RBT/nos3_luca_OISL/Backward_Sat/COSMOS_Control/Execution/OISL/files_received/plainText.txt";

static const char* back_alignment_mem_info = "/mnt/extras/SSD/NOS3_RBT/nos3_local_OISL/components/oisl/fsw/src/B_sat_for_alignment.txt";
static const char* for_alignment_mem_info = "/mnt/extras/SSD/NOS3_RBT/nos3_local_OISL/components/oisl/fsw/src/F_sat_back_alignment.txt";

const double    transfer_time_to_add = 0.0;    // around 3.52 GB.  Used only for Large file transfer to simulate a larger size
const int       forbidden_direction = 5;         // Forbidden direction for routing to avoid ping pong: TODO: improve making it smarter
const char *OGS_ASSUMED = "Igrim";               // For large file transfers since the OGS name is written only in the header file

/************************************************************
 *             Routing structures and margins               *
 ************************************************************/

static const double margin  = 60.0;           // Margin to align with the OGS. If the sat is already in OGS Mode the margin is 0. If it has to get into that mode then it might be even higher. TODO add autoregolation based on the mode you are in now
static const double margin_routing = 120.0;   // time to align with receiver + time for the receiver to eventually align with OGS/another receiver
static const int    marginDL = 10;            // This is the margin assuming alignment achieved, it is used to quantify how much data could be transfered during a pass. Assume 10 seconds of loss due to atmospheric conditions 

/************************************************************
 *                      Simulated Network Functions         *
 ************************************************************/

const int networkDelay = 7000;                // 7 ms ONE TRIP

void simulateNetworkDelay(void) {
    usleep(networkDelay); // Simulate network delay for a OISL distance of around 2000km -> 7 ms
    // TODO: How to implement this as 100ms sim time, instead of real time?
}

int sendPDU(CF_CFDP_PduFileDataHeader_t *header, CF_CFDP_PduFileDataContent_t *content, int segmentNumber, const char *fileContent, int segmentSize) {
    // Simulate a random chance of transmission failure (e.g., 10% failure rate)
    double failureRate = 0.1;
    double randomValue = (double)rand() / RAND_MAX;

    if (randomValue < failureRate) {
        return 0; // Indicate failure
    }
    printf("Sending PDU #%d \n", segmentNumber);
    printf("\"%.*s\"\n", segmentSize, content->data);
    // Here you would add the code to actually send the PDU over the network
    return 1; // Assume it always succeeds for this example
}

int receivePDU(CF_CFDP_PduFileDataHeader_t *receivedHeader, CF_CFDP_PduFileDataContent_t *receivedContent, int segmentNumber) {
    // Simulate checking the integrity of the received PDU
    // This can include checking a checksum or other error-detecting mechanism
    int pduValid = 1; // For simplicity, assume the PDU is valid (1 = valid, 0 = invalid)

    // Print received PDU data
    printf("Received PDU #%d\n", segmentNumber);
    printf("\"%.*s\"\n", (int)(CF_MAX_PDU_SIZE - sizeof(CF_CFDP_PduFileDataHeader_t)), receivedContent->data);

    if (pduValid) {
        printf("PDU #%d is valid. Sending ACK.\n", segmentNumber);
        return 1; // Indicates the PDU is valid and ready for ACK
    } else {
        printf("PDU #%d is invalid. No ACK will be sent.\n", segmentNumber);
        return 0; // Indicates the PDU is invalid
    }
}

CF_CFDP_PduAck_t createAck(CF_CFDP_FileDirective_t dir_code, CF_CFDP_ConditionCode_t cc, int segmentNumber) {
    CF_CFDP_PduAck_t ack;
    ack.directive_and_subtype_code.octets[0] = (uint8)((dir_code << 4) | 1);                          // Directive and subtype code
    ack.cc_and_transaction_status.octets[0] = (uint8)((cc << 4) | CF_CFDP_TransactionStatus_SUCCESS); // Condition code and transaction status

    // For simplicity, print ACK details
    // printf("Created ACK for PDU #%d\n", segmentNumber);
    // printf("Directive and subtype code: %u\n", (unsigned int)ack.directive_and_subtype_code.octets[0]);
    // printf("Condition code and transaction status: %u\n", (unsigned int)ack.cc_and_transaction_status.octets[0]);

    return ack;
}

void sendAck(CF_CFDP_PduAck_t *ack) {
    // Simulate sending ACK (in reality, you'd send this over the network)
    printf("Sending ACK with directive and subtype code: %d, condition code and transaction status: %d\n",
           ack->directive_and_subtype_code.octets[0], ack->cc_and_transaction_status.octets[0]);
}

int receiveAck(int segmentNumber, CF_CFDP_PduAck_t *ack) {
    // TODO: This must come from the other satellite
    // TODO: IMOPLEMENT A TIMEOUT. IF THE ACK IS NOT RECEIVED WITHIN TIME OUT, ASSUME FAILURE
    // Simulate a random chance of ACK delay or loss
    // Simulate verifying the ACK
    if (ack->cc_and_transaction_status.octets[0] == CF_CFDP_TransactionStatus_SUCCESS) {
        printf("Received valid ACK for PDU #%d\n", segmentNumber);
        return 1; // Valid ACK received
    } else {
        printf("Received invalid or error ACK for PDU #%d\n", segmentNumber);
        return 0; // Invalid ACK, might need to retransmit
    }
}

/************************************************************
 *                     Helper Functions                     *
 ************************************************************/

int segmentFileIntoPDUs(const char *fileContent, size_t fileSize, CF_CFDP_PduFileDataHeader_t **headers, CF_CFDP_PduFileDataContent_t **contents, int segmentSize) {
    int segmentCount = 0;
    int length = fileSize;
    int i;

    // Calculate the number of segments needed for this specific iteration size
    segmentCount = (fileSize + segmentSize - 1) / segmentSize;

    // Allocate memory for headers and contents dynamically
    *headers = (CF_CFDP_PduFileDataHeader_t *)malloc(segmentCount * sizeof(CF_CFDP_PduFileDataHeader_t));
    *contents = (CF_CFDP_PduFileDataContent_t *)malloc(segmentCount * sizeof(CF_CFDP_PduFileDataContent_t));

    if (*headers == NULL || *contents == NULL) {
        // Handle memory allocation failure
        fprintf(stderr, "Memory allocation failed\n");
        return -1; // Indicate failure
    }

    for (i = 0; i < length; i += segmentSize) {
        // Fill the header with the correct offset
        (*headers)[i / segmentSize].offset.octets[0] = (i >> 24) & 0xFF;
        (*headers)[i / segmentSize].offset.octets[1] = (i >> 16) & 0xFF;
        (*headers)[i / segmentSize].offset.octets[2] = (i >> 8) & 0xFF;
        (*headers)[i / segmentSize].offset.octets[3] = i & 0xFF;

        // Copy the segment data into the content structure
        int currentSegmentSize = (i + segmentSize > length) ? (length - i) : segmentSize; // Handle last segment which might be smaller
        strncpy((char *)(*contents)[i / segmentSize].data, &fileContent[i], currentSegmentSize);
    }
    return segmentCount;
}

double estimateTransferTime(size_t fileSize, int segmentCount) {
    // Convert file size to bits
    double fileSizeBits = fileSize * 8.0; 
    // Convert Mbps to bps
    double transferSpeedBps = transferSpeedMbps * 1e6; 
    // Time in seconds
    double estimatedTransferTimeSeconds = fileSizeBits / transferSpeedBps; 
    // Get segmentCount to take into consideration the Network delay
    double delay = segmentCount * (networkDelay / 1e6); // seconds

    return estimatedTransferTimeSeconds + delay;
}

time_t get_current_time(void) {
    // Base time for the simulation (2025-10-18 08:30:00 UTC)
    struct tm base_time = { .tm_year = 2025 - 1900, .tm_mon = 10 - 1, .tm_mday = 18,
                            .tm_hour = 8, .tm_min = 30, .tm_sec = 0, .tm_isdst = -1 };
    time_t base_timestamp = mktime(&base_time);

    // Retrieve current simulation time in seconds and subseconds
    CFE_TIME_SysTime_t nowT = CFE_TIME_GetTime();
    uint32_t seconds = nowT.Seconds;
    uint32_t subseconds = nowT.Subseconds;

    // Convert to full timestamp in seconds
    double sim_seconds = (double)seconds + ((double)subseconds / 4294967296.0);
    return base_timestamp + (time_t)sim_seconds;
}

void createSentFile(const char *fileContent, const char *fileMemn) {
    // TODO source and dest must be defined by the file content.
    FILE *sentFile = fopen(fileSent_confirmation, "w");
    if (sentFile != NULL) {
        if (strcmp(fileMemn, "OGS") == 0) {
            time_t current_time = get_current_time();
            struct tm *timeinfo = localtime(&current_time);  // Convert to local time
            char buffer[20]; // Buffer to store formatted time
            strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
            fprintf(sentFile, "%s %s\n", Sat_Name, buffer);  // Write the timestamp to the file
        }
        // Write the entire file content to the confirmation file
        fprintf(sentFile, "%s", fileContent);
        fclose(sentFile);
        printf("File 'file_sent_confirmation.txt' created with the content copied from fileContent.\n");
    } else {
        printf("Error creating 'file_sent_confirmation.txt' file.\n");
    }
}

void createSentFile2(const char *fileContent, const int direction) { 
    // source and dest must be defined by the file content.
    FILE *sentFile = fopen(fileSent_confirmation, "w");
    if (sentFile != NULL) {
        // Write src and dest as the first line based on direction
        if (direction == 1) { 
            fprintf(sentFile, "%s %s\n", my_src, for_dest); 
        }
        else if (direction == 2) { 
            fprintf(sentFile, "%s %s\n", my_src, back_dest); 
        }
        else { // OGS DL
            time_t current_time = get_current_time();
            struct tm *timeinfo = localtime(&current_time);  // Convert to local time
            char buffer[20]; // Buffer to store formatted time
            strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
            fprintf(sentFile, "%s %s\n", Sat_Name, buffer);  // Write the timestamp to the file
        }
        // Write the entire file content to the confirmation file
        fprintf(sentFile, "%s", fileContent);
        fclose(sentFile);
        printf("File 'file_sent_confirmation.txt' created with the content copied from fileContent.\n");
    } else {
        printf("Error creating 'file_sent_confirmation.txt' file.\n");
    }
}

/************************************************************
 *                     OGS Visibility Functions             *
 ************************************************************/

// Function to extract the ground station name directly from the file content
void extractOGSName(const char *fileContent, char *OGS_name, size_t max_len) {
    char command[1000];
    FILE *fp;
    char buffer[500];

    FILE *temp_file = fopen("/mnt/extras/SSD/NOS3_RBT/nos3_local_OISL/components/oisl/fsw/src/ftemp.json", "w");
    if (temp_file != NULL) {
        fputs(fileContent, temp_file);
        fclose(temp_file);
        snprintf(command, sizeof(command),
                "python3 /mnt/extras/SSD/NOS3_RBT/nos3_local_OISL/components/oisl/fsw/src/OGS_name.py /mnt/extras/SSD/NOS3_RBT/nos3_local_OISL/components/oisl/fsw/src/ftemp.json");
    }

    printf("Command: %s\n", command);

    // Run the Python script and capture its output
    fp = popen(command, "r");
    if (fp == NULL) {
        fprintf(stderr, "Failed to run Python script\n");
    }

    // Read the output and parse the position vector
    if (fgets(buffer, sizeof(buffer) - 1, fp) != NULL) {
        if (sscanf(buffer, "OGS Name: %s", OGS_name) == 1) {
            // Successfully parsed the position vector
            printf("DEBUG: Got the OGS NAME\n");
        } else {
            // Print other output lines if any
            printf("FAILED.\n");
            printf("%s", buffer);
        }
    }
    pclose(fp);
}

// Function to check if a given time is within a visibility window
int is_visible(time_t current_time, const char *vis_start, const char *vis_end, time_t *vis_start_time, double fileTransferDur, double vis_duration) {
    struct tm tm_start, tm_end;
    time_t start_time, end_time;

     // Ensure tm_isdst is set to -1 to handle DST automatically TODO Prob delete
    tm_start.tm_isdst = -1;
    tm_end.tm_isdst = -1;

    // Temporary buffers to hold truncated visibility start and end times
    char vis_start_trunc[20];
    char vis_end_trunc[20];

    // Copy only the first 19 characters (ignoring fractional seconds)
    strncpy(vis_start_trunc, vis_start, 19);
    vis_start_trunc[19] = '\0';
    strncpy(vis_end_trunc, vis_end, 19);
    vis_end_trunc[19] = '\0';

    // Parse visibility start time
    if (strptime(vis_start, "%Y-%m-%dT%H:%M:%S", &tm_start) == NULL) {
        printf("Failed to parse vis_start: %s\n", vis_start_trunc);
        return 0;
    }

    // Parse visibility end time
    if (strptime(vis_end, "%Y-%m-%dT%H:%M:%S", &tm_end) == NULL) {
        printf("Failed to parse vis_end: %s\n", vis_end_trunc);
        return 0;
    }

    // Convert to time_t format for comparison
    start_time = mktime(&tm_start);
    end_time = mktime(&tm_end);

    // Check if mktime failed
    if (start_time == -1) {
        printf("mktime failed to convert start_time.\n");
        printf("start_time: %ld, end_time: %ld\n", start_time, end_time);
        return 0;
    }
    if (end_time == -1) {
        printf("mktime failed to convert end_time.\n");
        return 0;
    }

    // Update the next upcoming visibility start time if conditions are met
    if (start_time > current_time && (*vis_start_time == -1 || start_time < *vis_start_time)) {
        *vis_start_time = start_time;
    }

    // Adjust the end time by subtracting margin and file transfer duration
    time_t adjusted_end_time = end_time - (time_t)(margin + fileTransferDur);

    // Check if the current time is within the adjusted visibility window
    return ((current_time >= start_time && current_time <= adjusted_end_time) && (vis_duration - margin - marginDL >= fileTransferDur));
}

/************************************************************
 *                     Routing Functions                    *
 ************************************************************/

int determine_direction(int sat_index, int central_index, int total_satellites) {
    // Normalize satellite indices to handle wrap-around
    int normalized_sat = (sat_index - central_index + total_satellites) % total_satellites;
    
    if (normalized_sat == 0) {
        return 0; // Central satellite
    } else if (normalized_sat <= total_satellites / 2) {
        return 1; // Forward direction
    } else {
        return 2; // Backward direction
    }
}

int adjust_candidate_capacities(SatCandidate *candidates, int candidate_count, 
                              time_t current_time, double fileTransferDur) {
                                
    #ifdef ROUTING_LOGGING
        printf("\n=== Starting Capacity Adjustment ===\n");
    #endif
    
    // Step 0: Calculate total bytes to transfer for the complete file
    double total_bytes = fileTransferDur * TransferCapacityperSecond;
    #ifdef ROUTING_LOGGING
        printf("Total bytes to transfer: %.2f GB\n", total_bytes/1e9);
    #endif
    
    // Step 1: Calculate total GB in each routing direction
    double total_forward = 0;   // Total bytes allocated to forward-direction satellites
    double total_backward = 0;  // Total bytes allocated to backward-direction satellites
    for (int i = 0; i < candidate_count; i++) {
        if (candidates[i].direction == 1) {
            total_forward += candidates[i].allocated_bytes;
        } else if (candidates[i].direction == 2) {
            total_backward += candidates[i].allocated_bytes;
        } 
    }
    // Calculate remaining data for central satellite (if any)
    double total_central = total_bytes - (total_backward + total_forward);
    
    #ifdef ROUTING_LOGGING
        printf("Initial forward allocation: %.2f GB\n", total_forward/1e9);
        printf("Initial central allocation: %.2f GB\n", total_central/1e9);
        printf("Initial backward allocation: %.2f GB\n", total_backward/1e9);
    #endif

    // Determine primary routing direction (based on first candidate)
    int dir_principale = candidates[0].direction;
    
    // Running total of excess bytes that need reallocation
    double excess_bytes = 0;
    
    // Step 2-4: Process each satellite and adjust allocations based on timing constraints
    for (int i = 0; i < candidate_count; i++) {
        SatCandidate *curr = &candidates[i];

        #ifdef ROUTING_LOGGING
            printf("\n--- Processing Satellite %d ---\n", curr->sat_index);
        #endif
        
        if (curr->allocated_bytes <= 0 && excess_bytes <= 0) {
            #ifdef ROUTING_LOGGING
                printf("Satellite has no allocation and no excess bytes, skipping\n");
            #endif
            continue;
        }
        
        // Calculate time available before this satellite's visibility window starts
        double time_available = difftime(curr->visibility_start, current_time);
        #ifdef ROUTING_LOGGING
            printf("Time available before visibility start: %.2f s\n", time_available);
        #endif

        // Compute the time needed to route data to this satellite based on its position and direction
        double transfer_bytes = 0;  // Total bytes that must be transferred to reach this satellite
        double base_routing_time = 0;  // Minimum time needed for signal propagation
        
        // Case 1: Central satellite
        if (curr->direction == 0) {
            #ifdef ROUTING_LOGGING
                printf("Case central\n");
            #endif
            
            // For central satellites, account for potential bidirectional routing
            if (total_backward >= 0 && total_forward >= 0)  {
                // Both directions are active, need time for both routing paths
                base_routing_time = 2 * margin_routing;
            } else {
                // Only one direction active
                base_routing_time = margin_routing;
            }
            
            #ifdef ROUTING_LOGGING
                printf("Base routing %f\n", base_routing_time);
            #endif
            
            // Central satellites need to handle all data coming from both directions
            transfer_bytes = total_forward + total_backward;
            
            #ifdef ROUTING_LOGGING
                printf("Transfer bytes is %f GB\n", transfer_bytes/1e9);
            #endif
        }
        // Case 2: Forward satellite when forward is the primary direction
        else if (curr->direction == 1 && dir_principale == 1) {
            #ifdef ROUTING_LOGGING
                printf("Case foward main direction\n");
            #endif
            
            // Base routing time is proportional to hop count
            base_routing_time = curr->hops * margin_routing;
            
            #ifdef ROUTING_LOGGING
                printf("Base routing %f\n", base_routing_time);
            #endif
            
            // Start with all forward data that must pass through this path
            transfer_bytes = total_forward;
            double tot_forward = total_forward;
            int numb_hops_curr = curr->hops;
            
            // For multi-hop paths, account for data that's dropped off at intermediate satellites
            for (int j = 1; j < numb_hops_curr; j++) {
                for (int i = 0; i < candidate_count; i++) {
                    if (candidates[i].direction == 1 && candidates[i].hops == j) {
                        // For each intermediate hop, subtract data that's already been delivered
                        transfer_bytes += tot_forward - candidates[i].allocated_bytes;
                        tot_forward -= candidates[i].allocated_bytes;
                    }
                }
            }
            
            #ifdef ROUTING_LOGGING
                printf("Trasnfer bytes is %f\n", transfer_bytes/1e9);
            #endif
        }
        // Case 3: Backward satellite when forward is the primary direction
        else if (curr->direction == 2 && dir_principale == 1) {
            #ifdef ROUTING_LOGGING
                printf("Case backward with main dir forward\n");
            #endif
            
            // For backward satellites, add extra hop for direction change
            base_routing_time = curr->hops * margin_routing + margin_routing;
            
            #ifdef ROUTING_LOGGING
                printf("Base routing %f\n", base_routing_time);
            #endif
            
            // Need to account for both forward and backward data
            transfer_bytes = total_backward + total_forward;
            double tot_backward = total_backward;
            int numb_hops_curr = curr->hops;
            
            // Similar to forward case, account for data delivered at intermediate hops
            for (int j = 1; j < numb_hops_curr; j++) {
                for (int i = 0; i < candidate_count; i++) {
                    if (candidates[i].direction == 2 && candidates[i].hops == j) {
                        transfer_bytes += tot_backward - candidates[i].allocated_bytes;
                        tot_backward -= candidates[i].allocated_bytes;
                    }
                }
            }
            
            #ifdef ROUTING_LOGGING
                printf("Total bytes is %f\n", transfer_bytes/1e9);
            #endif
        }
        // Handle unexpected cases
        else {
            printf("ADJUST CANDIDATE CAPACITIES [ERROR]: NOT IMPLEMENTED YET BUT SHOULD NOT BE HERE\n");
        }

        // Convert transfer bytes to time based on downlink capacity
        double transfer_time = transfer_bytes / TransferCapacityperSecond; 

        // Total time needed to route data to this satellite
        // Note: This excludes the satellite's own data transfer time
        double time_needed_reach = base_routing_time + transfer_time; 

        #ifdef ROUTING_LOGGING
            printf("Time needed to reach this satellite: %.2f s\n", time_needed_reach);
        #endif
        
        // Check if we need to adjust allocation based on timing constraints
        if (time_needed_reach > time_available) {
            // Not enough time to reach this satellite before visibility window
            // Calculate how much data needs to be reduced
            double time_loss = time_needed_reach - time_available;
            double excess = time_loss * TransferCapacityperSecond;
            
            // Adjust reduction based on hop count (multi-hop paths require less reduction)
            int number_hops = fmax(curr->hops, 1);
            excess = excess / number_hops;
            
            // Ensure we don't reduce more than what's allocated
            excess = fmin(excess, curr->allocated_bytes);
            
            // Calculate new allocation
            double new_allocated = curr->allocated_bytes - excess;
            
            #ifdef ROUTING_LOGGING
                printf("Reducing allocation from %.2f GB to %.2f GB\n",    
                    curr->allocated_bytes/1e9, new_allocated/1e9);
            #endif
            
            // Update satellite allocation and tracking variables
            curr->allocated_bytes = new_allocated;
            excess_bytes += excess;
            
            // Update direction totals to maintain consistency
            if (curr->direction == 1) {
                total_forward -= excess;
            } else if (curr->direction == 2) {
                total_backward -= excess;
            } else {
                total_central -= excess;
            }
            
            #ifdef ROUTING_LOGGING
                printf("Debug: now excess is: %f and the total forward is %f\n", excess_bytes, total_forward);
            #endif
        } 
        // If this satellite has capacity and we have excess data to distribute
        else if (excess_bytes > 0) {
            // Calculate how much additional data this satellite can handle
            double extra_time = time_available - time_needed_reach;
            double max_extra_bytes = (extra_time * TransferCapacityperSecond);
            double bytes_to_add = fmin(excess_bytes, max_extra_bytes);
            
            // Ensure we don't exceed satellite's downlink capacity
            if (bytes_to_add + curr->allocated_bytes > curr->dl_capacity) {
                bytes_to_add = curr->dl_capacity - curr->allocated_bytes;
                
                if (bytes_to_add < 0) {
                    printf("WTF THIS IS WRONGGGG\n");
                }
               
            }
            
            #ifdef ROUTING_LOGGING
                printf("Adding %.2f GB from excess to current allocation of %.2f GB\n",
                    bytes_to_add/1e9, curr->allocated_bytes/1e9);
            #endif
            
            // Update satellite allocation and tracking variables
            curr->allocated_bytes += bytes_to_add;
            excess_bytes -= bytes_to_add;
            
            // Update direction totals to maintain consistency
            if (curr->direction == 1) {
                total_forward += bytes_to_add;
            } else if (curr->direction == 2) {
                total_backward += bytes_to_add;
            } else {
                total_central += bytes_to_add;
            }
        }
        
        // Update satellite's transfer ratio for file-size percentage calculations
        curr->transfer_ratio = (curr->allocated_bytes / TransferCapacityperSecond) / fileTransferDur;
    }
    
    // Step 6: If there are still excess bytes, try to assign to satellites with zero allocation
    if (excess_bytes > 0) {
        #ifdef ROUTING_LOGGING
            printf("\n=== Attempting to allocate remaining %.2f GB ===\n", excess_bytes/1e9);
        #endif
        
        for (int i = 0; i < candidate_count && excess_bytes > 0; i++) {
            if (candidates[i].allocated_bytes <= 0) {
                // Calculate time constraints for this unallocated satellite
                double time_available = difftime(candidates[i].visibility_start, current_time);
                double base_routing_time = candidates[i].hops * margin_routing;
                double usable_time = time_available - base_routing_time;
                
                // Only allocate if there's enough time to route to this satellite
                if (usable_time > 0) {
                    // Calculate maximum possible allocation considering hop count
                    double max_possible = (usable_time * TransferCapacityperSecond) / candidates[i].hops;
                    double bytes_to_add = fmin(excess_bytes, max_possible);
                    
                    // Update satellite allocation
                    candidates[i].allocated_bytes = bytes_to_add;
                    candidates[i].transfer_ratio = (bytes_to_add / TransferCapacityperSecond) / fileTransferDur;
                    excess_bytes -= bytes_to_add;
                    
                    #ifdef ROUTING_LOGGING
                        printf("Allocated %.2f GB to previously empty satellite %d\n",
                            bytes_to_add/1e9, candidates[i].sat_index);
                    #endif
                }
            }
        }
    }
    
    // Remove satellites with zero allocation and compact the array
    int new_count = 0;
    for (int i = 0; i < candidate_count; i++) {
        if (candidates[i].allocated_bytes > 0) {
            if (i != new_count) {
                candidates[new_count] = candidates[i];
            }
            new_count++;
        }
    }
    
    #ifdef ROUTING_LOGGING
        printf("\n=== Final Allocation State ===\n");
        for (int i = 0; i < new_count; i++) {
            printf("Sat %d: %.2f GB\n", candidates[i].sat_index, 
                candidates[i].allocated_bytes/1e9);
        }
    #endif
    if (excess_bytes > 0) {
        printf("ADJUST CAPACITY [ERROR]: %.2f GB could not be allocated\n", excess_bytes/1e9);
    }
    
    return new_count;
}

int routing_Sat(FILE *vis_file, time_t current_time, time_t *start_visibility, double fileTransferDur, SplittingInfo *info) {  
    char line[300];
    char sat_id[20], vis_start[20], vis_end[20];
    double duration;
    int recommend_direction = 0;
    int central_index;

    SatCandidate candidates[MAX_CANDIDATES];
    int candidate_count = 0;

    // Extract central satellite index from the Sat_Name global variable
    sscanf(strrchr(Sat_Name, '_') + 1, "%d", &central_index);
    rewind(vis_file);

    // First step: Collect all viable candidates  
    while (fgets(line, sizeof(line), vis_file) != NULL) {
        if (sscanf(line, "%20[^,],%20[^,],%20[^,],%lf", sat_id, vis_start, vis_end, &duration) == 4) {  // TODO this would become  == 5 with DL CAPACITY already in the visibility prediction file
            char *trimmed_sat_id = strtok(sat_id, " \t\n\r");
            struct tm tm_start;
            time_t start_time;
            tm_start.tm_isdst = -1;
            vis_start[19] = '\0';

            // Calculate downlink capacity for this visibility window
            // Subtract margins from both sides of the visibility window
            // IMP: This step gets rid of candidates whose pass is shorter than a treshold: 70 seconds (margin + marginDL)
            double dlCapacity = ((duration - margin - marginDL) * TransferCapacityperSecond > 0) ?
                              (duration - margin - marginDL) * TransferCapacityperSecond : 0;  // dlCapacity in bytes

            if (dlCapacity > 0) {
                // Convert visibility start time string to time_t
                strptime(vis_start, "%Y-%m-%dT%H:%M:%S", &tm_start);
                start_time = mktime(&tm_start);

                if (start_time == -1) {
                    printf("ROUTING: mktime failed to convert start_time.\n");
                    continue;
                }
                
                // Extract satellite index from the id (format: "Sat_X" where X is the index)
                int sat_index = atoi(strrchr(trimmed_sat_id, '_') + 1);

                // Determine direction
                int direction = determine_direction(sat_index, central_index, MAX_CANDIDATES);

                // Skip satellites in forbidden direction to prevent routing loops. TODO: dirty fix, improve it
                if (direction == forbidden_direction) {
                    continue;
                }

                // Calculate minimum hops needed to reach this satellite
                // The constellation is circular, so we take the minimum distance in both directions
                int hops_needed = abs(sat_index - central_index);
                if (hops_needed > MAX_CANDIDATES / 2) { // Assuming 24 satellites in the same orbital ring
                    hops_needed = MAX_CANDIDATES - hops_needed;
                }

                // Calculate time needed to align the routing path (proportional to hop count)
                time_t alignment_time = hops_needed * (time_t)margin_routing;

                // Only consider satellites that will be visible after we can route to them
                if (start_time > current_time + alignment_time) {

                    // Calculate transfer ratio (how much of the file can be transferred in this window)
                    double transfer_ratio = (duration - marginDL - margin) / fileTransferDur;

                    // Check if the sat_index is already in the candidates list
                    int already_exists = 0;
                    for (int i = 0; i < candidate_count; i++) {
                        if (candidates[i].sat_index == sat_index) {
                            already_exists = 1;  // If the sat_index is already in the list, skip adding it
                            break;
                        }
                    }

                    // Add to candidates array
                    if (!already_exists && candidate_count < MAX_CANDIDATES) {
                        candidates[candidate_count].sat_index = sat_index;
                        candidates[candidate_count].direction = direction;
                        candidates[candidate_count].visibility_start = start_time;
                        candidates[candidate_count].transfer_ratio = transfer_ratio;
                        candidates[candidate_count].hops = hops_needed;
                        candidates[candidate_count].dl_capacity = dlCapacity;
                        candidates[candidate_count].allocated_bytes = 0;
                        candidate_count++;
                    }
                }
            }
        }
    }

    // Second step: Analyze the list of candidates to determine the optimal routing strategy.
    if (candidate_count > 0) {
        // Sort candidates by visibility start time (earliest first)
        for (int i = 0; i < candidate_count - 1; i++) {
            for (int j = 0; j < candidate_count - i - 1; j++) {
                if (candidates[j].visibility_start > candidates[j + 1].visibility_start) {
                    SatCandidate temp = candidates[j];
                    candidates[j] = candidates[j + 1];
                    candidates[j + 1] = temp;
                }
            }
        }

        // First check if the earliest visible satellite can handle the entire file transfer
        // This avoids unnecessary splitting for small files, i.e. files that can be transferred quickly, below around 1 GB of size
        if (candidates[0].transfer_ratio >= 1.0) {
                // Found a satellite that can handle the entire file
                printf("Found this satellite for the routing to DL the entire data: %d", candidates[0].sat_index);
                *start_visibility = candidates[0].visibility_start;
                return candidates[0].direction;
        }
        
        // If we get here, we need to split the file
        double total_transfer_ratio = 0.0;

        #ifdef ROUTING_LOGGING
            printf("\nFile too large to handle: File splitting analysis:\n");
            printf("Available reachable satellites for partial transfers:\n");
        #endif

        // Calculate total available capacity
        double total_capacity = 0.0;
        for (int i = 0; i < candidate_count; i++) {
            total_capacity += candidates[i].dl_capacity;
            total_transfer_ratio += candidates[i].transfer_ratio;

            // Print details of each candidate satellite
            #ifdef ROUTING_LOGGING
                printf("Sat %d: Direction=%d, Start=%ld, Capacity=%.2f GB, TR=%.2f, Hops=%d Allocated bytes=%f\n",
                    candidates[i].sat_index,
                    candidates[i].direction,
                    candidates[i].visibility_start,
                    candidates[i].dl_capacity / 1e9,  // DL CAPACITY IN GB
                    candidates[i].transfer_ratio,
                    candidates[i].hops,
                    candidates[i].allocated_bytes);
            #endif
        }

        // If combined capacity is sufficient
        if (total_transfer_ratio >= 1.0) {

            // Initialize SplittingInfo structure to hold file splitting strategy
            info->direction = malloc(candidate_count * sizeof(int));
            info->segment_sizes = malloc(candidate_count * sizeof(double));
            info->sat_indexes = malloc(candidate_count * sizeof(char)); // Assuming char for simplicity, you can change to int if needed
            info->num_segments = 0;

            #ifdef ROUTING_LOGGING
                printf("\nFile can be split across multiple satellites.\n");
                printf("Total available capacity: %.2f GB\n", total_capacity / 1e9);
                printf("Recommended sequence:\n");
            #endif

            // Select the first satellite in the sequence as our routing target --> earlier visibility
            *start_visibility = candidates[0].visibility_start;
            recommend_direction = 3; // Code 3 indicates file splitting is needed.  
 
            // Calculate initial allocation of file portions to satellites
            // The adjust_candidate_capacities method is called later to adjust the allocation.
            double remaining_file = fileTransferDur;
            for (int i = 0; i < candidate_count && remaining_file > 0; i++) {

                // Allocate as much as possible to each satellite, up to their capacity
                double this_transfer = (candidates[i].dl_capacity < remaining_file * TransferCapacityperSecond) ? 
                                        candidates[i].dl_capacity : 
                                        remaining_file * TransferCapacityperSecond;
                
                candidates[i].allocated_bytes = this_transfer;
                
                #ifdef ROUTING_LOGGING
                    printf("Sat %d: %.2f GB (%.1f%%)\n",
                        candidates[i].sat_index,
                        this_transfer / 1e9,
                        (this_transfer / (fileTransferDur * TransferCapacityperSecond)) * 100);
                #endif 

                remaining_file -= this_transfer / TransferCapacityperSecond;
                
            }

            // Before adding the candidate to the Transfer program, see if the portion to transfer can be transferred in time. 

            // Adjust capacities considering routing constraints
            // This function may remove candidates that can't be reached in time
            int updated_count = adjust_candidate_capacities(candidates, candidate_count, current_time, fileTransferDur);
    
            // Update candidate_count with the new count
            candidate_count = updated_count;

            // Recalculate total capacity and transfer ratio
            total_transfer_ratio = 0.0;
            for (int i = 0; i < candidate_count; i++) {
                // printf("Debug sat %d this transfer ratio is: %f",candidates[i].sat_index, candidates[i].transfer_ratio);
                total_transfer_ratio += candidates[i].transfer_ratio;
                
            }

            // Verify if the adjusted allocation is still viable (99% since now the total_transfer_ratio is at exacly 1 and we want to avoid float problems)
            if (total_transfer_ratio >= 0.99) {
                // Process final satellite assignments for file splitting
                double remaining_file = fileTransferDur;
                for (int i = 0; i < candidate_count && remaining_file > 0; i++) {
                    double this_transfer = candidates[i].allocated_bytes;
                    
                    #ifdef ROUTING_LOGGING
                        printf("Sat %d: %.2f GB (%.1f%%)\n",
                            candidates[i].sat_index,
                            this_transfer / 1e9,
                            (this_transfer / (fileTransferDur *TransferCapacityperSecond)) * 100);
                    #endif

                    remaining_file -= this_transfer / TransferCapacityperSecond;

                    // Fill the SplittingInfo structure with the current satellite data
                    info->sat_indexes[info->num_segments] = candidates[i].sat_index;
                    info->segment_sizes[info->num_segments] = this_transfer; // bytes
                    // Determine direction (forward or backward)
                    info->direction[info->num_segments] = candidates[i].direction;
                    // Increment the number of segments
                    info->num_segments++;
                }

                // Resize arrays to final size to free unused memory
                info->sat_indexes = realloc(info->sat_indexes, info->num_segments * sizeof(char));
                info->segment_sizes = realloc(info->segment_sizes, info->num_segments * sizeof(double));
                info->direction = realloc(info->direction, info->num_segments * sizeof(int));

                
                // Print final splitting plan
                #ifdef ROUTING_LOGGING
                    for (size_t i = 0; i < info->num_segments; i++) {
                        printf("Sat %d: Direction=%d, Segment Size=%.2f GB\n",
                        info->sat_indexes[i],
                        info->direction[i],
                        info->segment_sizes[i] / 1e9);  // Convert segment size to GB
                    }
                #endif
            }
            else { 
                printf("\nERROR: Total transfer ratio not enough. SHould not be here\n");
            }
        
        }
        else {
            printf("\nERROR: Even with splitting, total capacity (%.2f) is insufficient for complete transfer\n",
                   total_transfer_ratio);
        }
    }

    return recommend_direction;
}

/************************************************************
 *                  Receiver memory functions               *
 ************************************************************/

bool waitForReceiverMemory(size_t requiredSize, const char* filename) {
    const int MAX_WAIT_TIME = 300;  // Maximum wait time in seconds
    int waitTime = 0;

    while (waitTime < MAX_WAIT_TIME) {
        // In a real implementation, this would be updated via inter-satellite communication
        receiveMemoryInfo(filename);
        if (receiverMemory.currentUsed <= receiverMemory.totalSize && receiverMemory.totalSize - receiverMemory.currentUsed >= requiredSize) {
            return true;
        }
        sleep(10);
        waitTime += 10;
        printf("Waiting for receiver memory to become available: %d/%d seconds\n",
               waitTime, MAX_WAIT_TIME);
    }
    return false;
}

void receiveMemoryInfo(const char* filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("ERROR: Filename for memory %s not valid, return\n", filename);
        return;  // Return without value since function is void
    }

    size_t mem_value;
    // int first_value;
    // Read both values properly
    if (fscanf(file, "%*d %zu", &mem_value) != 1) {
        fclose(file);
        printf("ERROR: Failed to read values or negative memory value: %zu\n", mem_value);
        return;
    }

    receiverMemory.currentUsed = mem_value;
    fclose(file);
}

/************************************************************
 *               Large File Transfer functions              *
 ************************************************************/

// Helper function to get file portion based on offset and size
FilePortion get_file_portion(const char* fileContent, size_t totalSize, size_t offset, size_t size, double sizePercentage, double offsetPercentage) {

    FilePortion portion = {NULL, 0, offset};
    #ifdef FILE_PORTION_LOGGING
        printf("get_file_portion [DEBUG]: I am about to extract a file portion of %zu from the file with total size: %zu\n", size, totalSize);
    #endif

    if (offset + size <= totalSize) {
        portion.data = malloc(size);
        if (portion.data) {
            memcpy(portion.data, fileContent + offset, size);
            portion.size = size;
        }
    }
    else {
        // If the requested portion is too large, calculate percentage-based size and offset: THIS HAPPENS IN CASE OF SIMULATED LARGE FILE TRANSFER
        size_t adjustedSize = (size_t)(sizePercentage * totalSize / 100.0);
        size_t adjustedOffset = (size_t)(offsetPercentage * totalSize / 100.0);

        #ifdef FILE_PORTION_LOGGING
            printf("get_file_portion [WARNING]: Requested size exceeds file size. Extracting %.2f%% of the file (%zu bytes) starting at %.2f%% (%zu bytes).\n",
                sizePercentage, adjustedSize, offsetPercentage, adjustedOffset);
        #endif

        // Ensure adjustedOffset + adjustedSize does not exceed totalSize
        if (adjustedOffset + adjustedSize > totalSize) {
            adjustedSize = totalSize - adjustedOffset; // Adjust size to fit within bounds
            #ifdef FILE_PORTION_LOGGING
                printf("get_file_portion [INFO]: Adjusted size to fit within file bounds: %zu bytes.\n", adjustedSize);
            #endif
        }

        if (adjustedSize > 0) {
            portion.data = malloc(adjustedSize);
            if (portion.data) {
                memcpy(portion.data, fileContent + adjustedOffset, adjustedSize);
                portion.size = adjustedSize;
                portion.offset = adjustedOffset; // Update the offset in the FilePortion struct
            }
        } else {
            printf("get_file_portion [ERROR]: Invalid percentage-based size calculation.\n");
        }
    }
    
    return portion;
}

void process_direction(int direction, double size, const uint8 mode, uint8_t *connection, 
                      const char *filename_mem, const char* fileContent, size_t totalSize, size_t offset, SplittingInfo* info, double fake_file_size, time_t vis_start_time) { // size is now in BYTES.
    
    // Skip processing if size is zero or negative
    if (size <= 0) return;
    
    #ifdef FILE_PORTION_LOGGING
        printf("\n=== Processing %d Distribution ===\n", mode);
        printf("Going into mode %d for %.2f GB transfer\n", mode, size/1e9);
        // DEBUG Print satellite sequence
        for (size_t i = 0; i < info->num_segments; i++) {
            if (info->direction[i] == direction) {
                printf("- Sat_%d: %.2f B\n", info->sat_indexes[i], info->segment_sizes[i]);  // segment_sizes is in byte!
            }
        }
    #endif

    size_t sizeInBytes = (size_t)(size); // Convert double to size_t
    double sizePercentage = (size / fake_file_size) * 100; 
    double offsetPercentage = ((double)offset / fake_file_size)* 100; 
    #ifdef FILE_PORTION_LOGGING
        printf("process_direction [Debug]: size and offset percentages: %f and %f", sizePercentage, offsetPercentage);
    #endif

    // Exctract the portion of file assigned to this direction/transfer
    FilePortion portion = get_file_portion(fileContent, totalSize, offset, sizeInBytes, sizePercentage, offsetPercentage);
    
    if (portion.data != NULL) {

        #ifdef FILE_PORTION_LOGGING
            printf("process_direction [DEBUG]: %d portion extracted.\n", mode);
        #endif

        // Transmit the portion
        // ADCS MODE will be changed
        Generic_ADCS_Mode_cmd_t cmd8;
        CFE_MSG_Init(CFE_MSG_PTR(cmd8.CmdHeader), CFE_SB_ValueToMsgId(GENERIC_ADCS_CMD_MID), sizeof(Generic_ADCS_Mode_cmd_t)); 
        CFE_MSG_SetFcnCode((CFE_MSG_Message_t *)&cmd8, GENERIC_ADCS_SET_MODE_CC);
        cmd8.Mode = mode;

        // Set OGS name if switching to ground alignment mode
        if (mode == 5) {
            strcpy(cmd8.OGS_Name, OGS_ASSUMED);
        }  

        CFE_SB_TimeStampMsg((CFE_MSG_Message_t *)&cmd8);  
        CFE_SB_TransmitMsg((CFE_MSG_Message_t *)&cmd8, true);

        // Segment the file portion into CFDP PDUs
        CF_CFDP_PduFileDataHeader_t *headers = NULL;
        CF_CFDP_PduFileDataContent_t *contents = NULL;
        int segmentCount = segmentFileIntoPDUs(portion.data, portion.size, &headers, &contents, segmentSize);

        // If downlinking to OGS, wait until visibility and alignment are confirmed
        if (direction == 0) {
            while (1) {
                // Continuously update current time
                time_t current_time = get_current_time();

                // Check conditions: OGSAlignment is 1 and current_time >= vis_start_time
                if (OISL_AppData.DevicePkt.Oisl.OGSAlignment == 1 && current_time >= vis_start_time) {
                    // Conditions are met, establish connection and exit loop
                    uint8 conn_est = 1;
                    connection = &conn_est;
                    break;
                }
                // Calculate the remaining time until the visibility window starts
                time_t time_to_vis_start = vis_start_time - current_time;
                #ifdef FILE_PORTION_LOGGING
                    // Print status message with current time and remaining wait time
                    printf("process_direction [DEBUG]: Waiting to enter the visibility window. The visibility start is: %ld and Time until start: %ld seconds\n", (long)vis_start_time, (long)time_to_vis_start);
                #endif
                sleep((int)time_to_vis_start);
            }
        }

        // Proceed to send the iteration

        // Simulate transfer duration based on segment size and link capacity
        double fake_duration = size / TransferCapacityperSecond;

        // Send the data and check for confirmation
        int confirmation = sendIteration(portion.data, portion.size, headers, contents, 
                                         segmentCount, segmentSize, connection, filename_mem, fake_duration);
        
        if (confirmation != 0) {
            printf("process_direction [ERROR]: Something went wrong during %d transfer\n", mode);
        } else {
            #ifdef FILE_PORTION_LOGGING
                printf("process_direction [DEBUG]: Iteration sent. Creating the SentFile\n");
            #endif
            createSentFile2(portion.data, direction);
        }

        // Free memory used for the file portion
        free(portion.data);
    }
    else {
        printf("process_direction [ERROR]: Portion of the file not extracted\n");
    }

    #ifdef FILE_PORTION_LOGGING
        printf("process_direction [DEBUG]: Finished transmitting %.2f GB to the %d direction.\n", size/1e9, mode);
    #endif
}

void handle_splitting(SplittingInfo* info, const char *fileContent, const size_t fileSize, time_t start_visibility) {  
    
    if (info == NULL || info->num_segments == 0) {
        printf("handle_splitting [ERROR]: Invalid splitting info\n");
        return;
    }

    // Calculate sizes and offsets for each direction
    double forward_size = 0.0;
    double backward_size = 0.0;
    double central_size = 0.0;
    size_t forward_offset = 0;
    size_t backward_offset = 0;
    size_t central_offset = 0;

    // Calculate total size for each direction and determine offsets
    for (size_t i = 0; i < info->num_segments; i++) {
        double size_bytes = info->segment_sizes[i];  // segment sizes are already in bytes

        switch (info->direction[i]) {
            case 1: // Forward
                forward_size += size_bytes;
                break;
            case 2: // Backward
                backward_size += size_bytes;
                break;
            case 0: // Central
                central_size += size_bytes;
                break;
            default:
                printf("handle_splitting [ERROR] Direction not recognized.\n");
                break;
        }
    }

    
    #ifdef FILE_PORTION_LOGGING
        printf("\nFile Distribution Analysis:\n");
        printf("Forward: %.2f GB\n", forward_size / 1e9);  
        printf("Backward: %.2f GB\n", backward_size / 1e9);
        printf("Central: %.2f GB\n", central_size / 1e9);
    #endif

    // Fake size because we simulate GBs of data but handle kBs of data
    double fake_file_size = forward_size + central_size + backward_size;    

    // Determine processing order based on priority
    int priority = info->direction[0];
    
    if (priority == 1) { // Forward first
        // Offset calculation
        backward_offset = forward_size + central_size;
        central_offset = forward_size;
        process_direction(1, forward_size, OISL_MODE_F, &OISL_AppData.DevicePkt.Oisl.ForwardConnection, 
                        "F_sat_back_alignment.txt", fileContent, fileSize, forward_offset, info, fake_file_size, start_visibility);
        process_direction(2, backward_size, OISL_MODE_B, &OISL_AppData.DevicePkt.Oisl.BackwardConnection, 
                        "B_sat_for_alignment.txt", fileContent, fileSize, backward_offset, info, fake_file_size, start_visibility);
        process_direction(0, central_size, OISL_MODE_OGS, &OISL_AppData.DevicePkt.Oisl.OGSAlignment, 
                        "OGS.txt", fileContent, fileSize, central_offset, info, fake_file_size, start_visibility);
    } else if (priority == 2) { // Backward first
        // Offset calculation
        forward_offset = forward_size + central_size;
        central_offset = backward_size;
        process_direction(2, backward_size, OISL_MODE_B, &OISL_AppData.DevicePkt.Oisl.BackwardConnection, 
                        "B_sat_for_alignment.txt", fileContent, fileSize, backward_offset, info, fake_file_size, start_visibility);
        process_direction(1, forward_size, OISL_MODE_F, &OISL_AppData.DevicePkt.Oisl.ForwardConnection, 
                        "F_sat_back_alignment.txt", fileContent, fileSize, forward_offset, info, fake_file_size, start_visibility);
        process_direction(0, central_size, OISL_MODE_OGS, &OISL_AppData.DevicePkt.Oisl.OGSAlignment, 
                        "OGS.txt", fileContent, fileSize, central_offset, info, fake_file_size, start_visibility);
    } else if (priority == 0) { // Central first
        // Offset calculation
        backward_offset = central_size;
        process_direction(2, backward_size, OISL_MODE_B, &OISL_AppData.DevicePkt.Oisl.BackwardConnection, 
                        "B_sat_for_alignment.txt", fileContent, fileSize, backward_offset, info, fake_file_size, start_visibility);
        process_direction(0, central_size, OISL_MODE_OGS, &OISL_AppData.DevicePkt.Oisl.OGSAlignment, 
                        "OGS.txt", fileContent, fileSize, central_offset, info, fake_file_size, start_visibility);
    }

}

// Helper function to free the splitting info structure
void free_splitting_info(SplittingInfo* info) {
    if (info != NULL) {
        if (info->direction != NULL) free(info->direction);
        if (info->segment_sizes != NULL) free(info->segment_sizes);
        if (info->sat_indexes != NULL) free(info->sat_indexes);
        free(info);
    }
}

int sendIteration(const char *fileContent, size_t fileSize, CF_CFDP_PduFileDataHeader_t *headers, CF_CFDP_PduFileDataContent_t *contents, int segmentCount, int segmentSize, uint8_t *connection_establishment, const char *filename_memory, double fake_duration) {

    // Check memory availability for OISL
    if (strcmp(filename_memory, "OGS") != 0) {
        if (*connection_establishment == 1) {
            receiveMemoryInfo(filename_memory);
            if (receiverMemory.currentUsed > receiverMemory.totalSize || receiverMemory.totalSize - receiverMemory.currentUsed < fileSize) {
                receiverMemory.isAvailable = false;
            }
        }

        if (receiverMemory.isAvailable == false) {
            #ifdef FILE_PORTION_LOGGING
                printf("sendIteration [DEBUG]: memory of the receiver not available. %zu. Will wait for the memory to free again. \n", receiverMemory.totalSize - receiverMemory.currentUsed);
            #endif
            if (!waitForReceiverMemory(fileSize, filename_memory)) {
                printf("sendIteration [ERROR]: Timeout waiting for receiver memory, aborting transfer\n");
                return -1; // Indicate failure
            }
        }
    }

    // Calculate realistic sleep time
    double sleep_time = fake_duration / segmentCount;
    #ifdef FILE_PORTION_LOGGING
        printf("sendIteration [DEBUG]: I will sleep %f after every PDUs sent to get to a fake duration of %f\n", sleep_time, fake_duration);
    #endif
    time_t start_segment_time;
    time_t end_segment_time;

    for (int segmentNumber = 0; segmentNumber < segmentCount; segmentNumber++) {
        int sent = 0;
        int retries = 0;
        const int maxRetries = 5;

        while (!sent && retries < maxRetries) {
            if (*connection_establishment == 1) {
                if (sent == 0) {
                    start_segment_time = get_current_time();
                    end_segment_time = start_segment_time + sleep_time;
                }
                *transferActive = 1;
                sent = sendPDU(&headers[segmentNumber], &contents[segmentNumber], segmentNumber, fileContent, segmentSize);
                if (sent == 1) {
                    simulateNetworkDelay();

                    // Simulate the other satellite receiving the PDU
                    if (receivePDU(&headers[segmentNumber], &contents[segmentNumber], segmentNumber)) {
                        CF_CFDP_PduAck_t ack = createAck(CF_CFDP_FileDirective_ACK, CF_CFDP_ConditionCode_NO_ERROR, segmentNumber);
                        sendAck(&ack);

                        // Simulate the sender receiving the ACK
                        if (!receiveAck(segmentNumber, &ack)) {
                            sent = 0;
                            retries++;
                            printf("sendIteration [WARNING]: PDU #%d not acknowledged, retrying (%d/%d)\n", segmentNumber, retries, maxRetries);
                        } else {
                            // Update memory usage
                            memoryInfo->currentUsed = (memoryInfo->currentUsed < (size_t)segmentSize) ? 0 : memoryInfo->currentUsed - segmentSize;
                            memoryInfo->isAvailable = (memoryInfo->currentUsed < memoryCapacity) ? 1 : 0;
                            // DEBUG TO SEE MEM CHANGING + TEST STABILITY 
                            while (start_segment_time < end_segment_time) {
                                sleep(2);
                                // update start segment time
                                start_segment_time = get_current_time();
                                // If it is the last segment, do not sleep. 
                                if (segmentNumber ==  segmentCount - 1) {start_segment_time = end_segment_time;}
                            }                    
                            // update end segment time
                            end_segment_time += sleep_time;
                        }
                    }
                } else {
                    retries++;
                    printf("sendIteration [WARNING]: PDU #%d failed to send, retrying (%d/%d)\n", segmentNumber, retries, maxRetries);
                }
            } else {
                printf("sendIteration [DEBUG]: Wait for re-alignment\n");
                if (*transferActive == 1) {
                    *transferActive = 0;
                }
                sleep(10); // Adjust sleep as needed
            }
        }

        if (retries == maxRetries) {
            printf("sendIteration [ERROR]: PDU #%d failed after %d retries, aborting transmission.\n", segmentNumber, maxRetries);
            if (*transferActive == 1) {
                *transferActive = 0;
            }
            return -1; // Indicate failure
        }
    }

    printf("sendIteration [DEBUG]: All PDUs sent successfully.\n");
    if (*transferActive == 1) {
        *transferActive = 0;
    }

    // Free dynamically allocated memory
    free(headers);
    free(contents);

    return 0; // Indicate success
}


void sendFile(const char *fileContent, const size_t fileSize) {
    // Log received file information
    printf("CFDP sendFile [INFO]: Received a file with size: %zu bytes\n", fileSize);

    // Allocate memory for PDU headers and contents
    CF_CFDP_PduFileDataHeader_t *headers = NULL;
    CF_CFDP_PduFileDataContent_t *contents = NULL;

    // Segment the file into PDUs for transmission
    int segmentCount = segmentFileIntoPDUs(fileContent, fileSize, &headers, &contents, segmentSize); 

    // Calculate expected transfer time including network delays
    double transferTime = estimateTransferTime(fileSize, segmentCount);
    transferTime += transfer_time_to_add; // AKA 3GB of file to DOWNLINK
    printf("CFDP sendFile [INFO]: Estimated (BIGGER) Transfer time including network delay: %f s.\n", transferTime);

    // Update local memory usage information
    memoryInfo->currentUsed += fileSize; // Bytes
    memoryInfo->isAvailable = (memoryInfo->currentUsed < memoryCapacity) ? 1 : 0;

    // Determine connection parameters based on target
    uint8 *connection_establishment;
    const char* filename_memory;

    // Handle different target scenarios (0=Backward, 1=Forward, 2=Ground Station: this scenario involves routing)
    if (OISL_AppData.CFDP.Target == 0) {
        // Target is backward satellite
        connection_establishment = &OISL_AppData.DevicePkt.Oisl.BackwardConnection;
        filename_memory = back_alignment_mem_info;
    }
    else if (OISL_AppData.CFDP.Target == 1) {
        // Target is forward satellite
        connection_establishment = &OISL_AppData.DevicePkt.Oisl.ForwardConnection;
        filename_memory = for_alignment_mem_info;
    }
    else if (OISL_AppData.CFDP.Target == 2) {
        // Target is Optical Ground Station (OGS)
        
        // Extract ground station name from file content
        char OGS_name[64];
        extractOGSName(fileContent, OGS_name, sizeof(OGS_name));
        printf("CFDP sendFile [INFO]: File received. OGS specified for the DL: %s\n", OGS_name);
        
        // Open visibility prediction file for the specified ground station
        char vis_file_name[200];
        snprintf(vis_file_name, sizeof(vis_file_name), 
                "/mnt/extras/SSD/NOS3_RBT/nos3_local_OISL/components/oisl/fsw/src/OGS_visibilities/%s_vis_prediction.txt", 
                OGS_name);
        FILE *vis_file = fopen(vis_file_name, "r");
        if (vis_file == NULL) {
            perror("Failed to open visibility prediction file");
        }

        char line[256];
        time_t vis_start_time = -1;
        int is_visible_now = 0;

        // Prepare ADCS mode change command
        Generic_ADCS_Mode_cmd_t cmd8;
        CFE_MSG_Init(CFE_MSG_PTR(cmd8.CmdHeader), CFE_SB_ValueToMsgId(GENERIC_ADCS_CMD_MID), sizeof(Generic_ADCS_Mode_cmd_t)); 

        time_t current_time = get_current_time();

        // Check visibility window status of the current satellite
        while (fgets(line, sizeof(line), vis_file) != NULL) {
            char sat_id[20], vis_start[20], vis_end[20];
            double duration;
            // Ensure line ends at '\n' and doesn't include any hidden characters
            line[strcspn(line, "\r\n")] = 0;

            int items = sscanf(line, "%20[^,],%20[^,],%20[^,], %lf", sat_id, vis_start, vis_end, &duration); 
            if (items == 4) {
                // Check if satellite is currently visible from ground station
                // vis_start_time stores the earliest visibility of the central sat that is larger than current time
                if (strcmp(sat_id, Sat_Name) == 0 && 
                    is_visible(current_time, vis_start, vis_end, &vis_start_time, transferTime, duration)) {   
                    printf("CFDP sendFile [INFO]: Satellite %s is currently visible from OGS.\n", sat_id);
                    char formatted_current[20];
                    strftime(formatted_current, sizeof(formatted_current), "%Y-%m-%d %H:%M:%S", localtime(&(current_time)));
                    is_visible_now = 1;
                    break;
                }
            }
        }

        // OGS Downlink: current satellite not visible (not having a pass) or pass too short --> look for routing!
        if (!is_visible_now) { 
            time_t my_vis_start = vis_start_time;           // Meaning, either it is not veasible, or the pass duration is too short.
            printf("CFDP sendFile [INFO]: Satellite Sat is not visible from OGS at this time.\n");
            
            // Create structure for file routing information
            SplittingInfo* info = malloc(sizeof(SplittingInfo));
            if (info == NULL) {
                printf("CFDP sendFile [ERROR]: ALLOCATION ERROR FOR SPLITTING INFO\n");
                return;
            }

            // Initialize routing info structure
            info->direction = NULL;
            info->segment_sizes = NULL;
            info->sat_indexes = NULL;
            info->num_segments = 0;

            // Determine optimal routing path within the orbital ring
            int routing = routing_Sat(vis_file, current_time, &vis_start_time, transferTime, info); 

            if (routing == 1) { 
                // Route forward - another satellite has earlier visibility
                connection_establishment = &OISL_AppData.DevicePkt.Oisl.ForwardConnection;
                filename_memory = for_alignment_mem_info;
                cmd8.Mode = OISL_MODE_F;
                printf("CFDP sendFile [INFO]: Routing forward\n");
            }
            else if (routing == 2) { 
                // Route backward
                connection_establishment = &OISL_AppData.DevicePkt.Oisl.BackwardConnection;
                filename_memory = back_alignment_mem_info;
                cmd8.Mode = OISL_MODE_B;
                printf("CFDP sendFile [INFO]: Routing backward\n");
            }
            else if (routing == 3 && info != NULL) {
                // File is too large for a single transfer - split into parts 
                handle_splitting(info, fileContent, fileSize, my_vis_start);
                free_splitting_info(info);
                printf("CFDP sendFile [INFO]: File has been splitted and each portion has been sent/downlinked. Concluded my work here.\n");
                return;
            }
            else { 
                // Direct downlink is best option - align with OGS and wait for visibility window
                cmd8.Mode = OISL_MODE_OGS;
                filename_memory = "OGS";
                strcpy(cmd8.OGS_Name, OGS_name);
                CFE_SB_TimeStampMsg((CFE_MSG_Message_t *)&cmd8);
                CFE_SB_TransmitMsg((CFE_MSG_Message_t *)&cmd8, true);

                // Wait until satellite enters visibility window
                while (1) {
                    current_time = get_current_time();

                    // Check if satellite is properly aligned and visibility window has started
                    if (OISL_AppData.DevicePkt.Oisl.OGSAlignment == 1 && current_time >= vis_start_time) {
                        // Conditions are met, establish connection and exit loop
                        uint8 conn_est = 1;
                        connection_establishment = &conn_est;
                        printf("Connection established to OGS.\n");
                        break;
                    }

                    // Calculate and display time remaining until visibility window
                    time_t time_to_vis_start = vis_start_time - current_time;
                    // Print status message with current time and remaining wait time
                    printf("CFDP sendFile [INFO]: Waiting to enter the visibility window. The visibility start is: %ld and Time until start: %ld seconds\n", (long)vis_start_time, (long)time_to_vis_start);
                    sleep((int)time_to_vis_start);
                }
            }
        } // Current sat not visible now

        else {
            // Satellite is visible now and download size is acceptable
            // Set up for direct downlink to OGS 
            strcpy(cmd8.OGS_Name, OGS_name);
            connection_establishment = &OISL_AppData.DevicePkt.Oisl.OGSAlignment;
            filename_memory = "OGS";
            cmd8.Mode = OISL_MODE_OGS;
        }

        // Transmit the MSG to modify ADCS mode
        CFE_SB_TimeStampMsg((CFE_MSG_Message_t *)&cmd8);
        CFE_SB_TransmitMsg((CFE_MSG_Message_t *)&cmd8, true);

        // Close the visibility file
        fclose(vis_file);

    } // CASE: DOWNLINK TO OGS

    else { 
        // Handle unknown target - default to forward routing
        printf("CFDP sendFile [ERROR]: Unknown taget to align with, or method not yet impemented for target %u, EXIT.", OISL_AppData.CFDP.Target);
        return;
    }

    // Use sendIteration to handle the actual file transmission
    int result = sendIteration(fileContent, fileSize, headers, contents, segmentCount, segmentSize, 
        connection_establishment, filename_memory, transferTime);

    // Process result of transmission
    if (result == 0) {
        printf("All PDUs sent successfully. File transmission is over \n");
        createSentFile(fileContent, filename_memory);
    } 
    else {
        printf("CFDP sendFile [ERROR]: File transmission incomplete.\n");
    }
}
