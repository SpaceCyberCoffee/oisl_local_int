#ifndef CFDP_H
#define CFDP_H

#include "CFDP_PDU.h"

// #define ROUTING_LOGGING
#define FILE_PORTION_LOGGING

/**
 * @brief Simulate network delay based on a specified delay time.
 */
void simulateNetworkDelay(void);

/**
 * @brief Sends a CFDP PDU (Protocol Data Unit).
 *
 * @param header Pointer to the PDU header.
 * @param content Pointer to the PDU content.
 * @param segmentNumber The number of the segment being sent.
 * @param fileContent The content of the file being sent.
 * @param segmentSize The size of the segment being sent.
 * @return int Returns 1 if the PDU was sent successfully, 0 otherwise.
 */
int sendPDU(CF_CFDP_PduFileDataHeader_t *header, CF_CFDP_PduFileDataContent_t *content, int segmentNumber, const char *fileContent, int segmentSize);

/**
 * @brief Receives a CFDP PDU and verifies its integrity.
 *
 * @param receivedHeader Pointer to the received PDU header.
 * @param receivedContent Pointer to the received PDU content.
 * @param segmentNumber The number of the segment being received.
 * @return int Returns 1 if the PDU is valid, 0 otherwise.
 */
int receivePDU(CF_CFDP_PduFileDataHeader_t *receivedHeader, CF_CFDP_PduFileDataContent_t *receivedContent, int segmentNumber);

/**
 * @brief Creates an ACK (Acknowledgment) PDU.
 *
 * @param dir_code The directive code of the PDU.
 * @param cc The condition code of the PDU.
 * @param segmentNumber The segment number for which the ACK is created.
 * @return CF_CFDP_PduAck_t Returns the created ACK PDU.
 */
CF_CFDP_PduAck_t createAck(CF_CFDP_FileDirective_t dir_code, CF_CFDP_ConditionCode_t cc, int segmentNumber);

/**
 * @brief Sends an ACK (Acknowledgment) PDU.
 *
 * @param ack Pointer to the ACK PDU to be sent.
 */
void sendAck(CF_CFDP_PduAck_t *ack);

/**
 * @brief Receives an ACK (Acknowledgment) PDU and verifies its integrity.
 *
 * @param segmentNumber The segment number associated with the ACK.
 * @param ack Pointer to the received ACK PDU.
 * @return int Returns 1 if the ACK is valid, 0 otherwise.
 */
int receiveAck(int segmentNumber, CF_CFDP_PduAck_t *ack);

/**
 * @brief Segments the file content into PDUs (Protocol Data Units).
 *
 * @param fileContent Pointer to the file content to be segmented.
 * @param fileSize The size of the file content.
 * @param headers Pointer to the array of PDU headers to be populated.
 * @param contents Pointer to the array of PDU contents to be populated.
 * @param segmentSize The size of each segment.
 * @return int Returns the number of segments created.
 */
int segmentFileIntoPDUs(const char *fileContent, size_t fileSize, CF_CFDP_PduFileDataHeader_t **headers, CF_CFDP_PduFileDataContent_t **contents, int segmentSize);

/**
 * @brief Estimates the total transfer time for a file.
 *
 * @param fileSize The size of the file in bytes.
 * @param segmentCount The total number of segments to be sent.
 * @return double Returns the estimated transfer time in seconds.
 */
double estimateTransferTime(size_t fileSize, int segmentCount);

/**
 * @brief Creates a confirmation file after sending data. Used to signal the server to move the file in the specified directory.
 *
 * This function writes a timestamp along with the satellite name (Sat_Name) to the first line 
 * of the confirmation file 'file_sent_confirmation.txt', but only if the destination is "OGS". 
 * Then, it writes the provided file content.
 *
 * @param fileContent The content to be written in the confirmation file.
 * @param fileMemn A string to identify if the file is being sent to an OGS ("OGS") or not.
 */
void createSentFile(const char *fileContent, const char *fileMemn);

/**
 * @brief Creates a confirmation file after sending data, with direction-aware source/destination. USED IN CASE OF ROUTING OF LARGE FILES
 *
 * This version supports inter-satellite links (forward/backward) and downlink to OGS.
 * Based on the `direction`, it writes a source-destination pair or timestamp as the first line,
 * followed by the full content of the file.
 *
 * @param fileContent The content to be written in the confirmation file.
 * @param direction Integer code indicating transmission direction:
 *        1 = forward link, 2 = backward link, any other = downlink to OGS.
 */
void createSentFile2(const char *fileContent, const int direction);

// Structure to track memory status
typedef struct
{
    size_t totalSize;
    size_t currentUsed;
    bool isAvailable;
} MemoryStatus;

extern MemoryStatus *memoryInfo; // declare as external so it can be accessed from device.c
extern uint8_t *transferActive;  // declare as external so it can be accessed from device.c

// Structure to hold file portion information
typedef struct {
    char* data;
    size_t size;
    size_t offset;
} FilePortion;

typedef struct {
    int *direction;         // Direction of transfer: 1, 2 or both 
    double *segment_sizes;  // Array to hold segment sizes in bytes
    char *sat_indexes;      // Index array.
    size_t num_segments;    // how many satellites will be active in DL info
} SplittingInfo;

// Structure to track multiple satellite candidates
typedef struct {
    int sat_index;
    int direction;
    time_t visibility_start;
    double transfer_ratio;
    int hops;
    double dl_capacity;        // Downlink capacity in bytes
    double allocated_bytes;    // bytes task to transfer
} SatCandidate;

/**
 * @brief Determines the relative direction of a satellite with respect to a central satellite.
 *
 * This function calculates whether a satellite is in the forward, backward,
 * or central position relative to a given central satellite index, accounting
 * for wrap-around in a circular constellation (e.g., ring topology).
 *
 * @param sat_index Index of the satellite to evaluate.
 * @param central_index Index of the reference (central) satellite.
 * @param total_satellites Total number of satellites in the constellation.
 * 
 * @return int Direction:
 *         0 = central satellite,
 *         1 = forward (moving in increasing index order),
 *         2 = backward (moving in decreasing index order).
 */
int determine_direction(int sat_index, int central_index, int total_satellites);

/**
 * adjust_candidate_capacities - Optimizes satellite capacity allocation for file splitting based on initial file division plan provided by routing_sat
 * 
 * This function adjusts the allocated bytes for each satellite candidate based on timing
 * constraints and routing efficiency. It ensures that:
 *   1. Each satellite receives data only when it's actually visible
 *   2. Data routing between satellites respects hop constraints and transfer times
 *   3. Satellite capacity is maximized while accounting for routing overhead
 *   4. Excess data is redistributed to satellites that can handle additional load
 * 
 * The algorithm models the propagation of data through the satellite network and accounts
 * for the cascading effects of data transfer from the central satellite to each candidate,
 * considering both forward and backward routing paths.
 * 
 * @param candidates      Array of satellite candidates provided by routing_sat
 * @param candidate_count Number of candidates in the array
 * @param current_time    Current sim timestamp when routing decisions are made
 * @param fileTransferDur Duration needed to transfer the complete file
 * 
 * @return Updated count of viable candidates after adjustment
 */
int adjust_candidate_capacities(SatCandidate *candidates, int candidate_count, time_t current_time, double fileTransferDur);

/**
 * routing_Sat - Determines optimal satellite routing strategy for file transfers
 * 
 * This function analyzes available satellite visibility windows to determine the best
 * routing strategy for transferring data. It can either:
 *   1. Find a single satellite capable of handling the entire file transfer
 *   2. Split the file across multiple satellites when no single satellite has sufficient capacity
 * 
 * The function considers factors such as:
 *   - Satellite GS visibility windows
 *   - Inter-satellite hop distances
 *   - Transfer ratios and DL capacities
 *   - Routing timing constraints
 *   - Direction constraints to prevent routing loops
 * 
 * First step: Collect all viable candidates for the routing, i.e. satellites with a pass duration above the margin (currently set to 70 s). Only earlliest visibility for each candidate.
 * Second step: Analyze the list of candidates to determine the optimal routing strategy. This includes
 * 2.1: check if one satellite in the list can handle the entire file transfer. If so, route in this direction.
 * 2.2: in the negative case, assing to each satellite a portion of file based on their DL capacities.
 * 2.3: in case of split, the adjust_candidate_capacities method is called.
 * Third step: return an id representing the routing direction.
 * 
 * @param vis_file          File pointer to OGS visibility data file
 * @param current_time      Current simulation time
 * @param start_visibility  Pointer to store the selected visibility start time. It is the start of the pass for the first (and possibly only) satellite in the routing path
 * @param fileTransferDur   Duration needed for complete file transfer
 * @param info              Pointer to structure for storing splitting information
 * 
 * @return Direction code for routing (0=none, 1=forward, 2=backward, 3=split)
 */
int routing_Sat(FILE *vis_file, time_t current_time, time_t *start_visibility, double fileTransferDur, SplittingInfo *info);

/**
 * waitForReceiverMemory - Waits until receiving satellite has sufficient memory available
 * 
 * This function polls the receiver's memory status and blocks until either:
 *   1. The receiving satellite has enough free memory to handle the transfer, or
 *   2. The maximum wait time is exceeded (timeout)
 * 
 * The function checks memory status by reading from a specified file that contains
 * memory information that would be updated via inter-satellite communication
 * in a real implementation.
 * 
 * @param requiredSize  Size of memory required for the transfer (in bytes)
 * @param filename      Path to the file containing receiver memory information
 * 
 * @return true if sufficient memory became available, false if timed out
 */
bool waitForReceiverMemory(size_t requiredSize, const char* filename);

/**
 * receiveMemoryInfo - Reads receiver memory status from a file
 * 
 * This function opens and parses a file containing memory information about
 * the receiving satellite. The file is expected to contain at least two values:
 * an identifier (which is ignored) and the current memory usage.
 * 
 * In a real implementation, this file would be updated via inter-satellite
 * communication to reflect the current memory state of the receiving satellite.
 * 
 * @param filename  Path to the file containing receiver memory information
 */
void receiveMemoryInfo(const char *filename);

/**
 * get_file_portion - Extracts a portion of a file based on offset and size parameters
 * 
 * This function creates a FilePortion structure containing a segment of the provided
 * file content. It handles two modes of operation:
 *   1. Absolute mode: Uses exact offset and size values to extract a specific byte range
 *   2. Percentage mode: Falls back to percentage-based calculations if absolute values
 *      would exceed the file boundaries. THIS HAPPENS IN CASE OF SIMULATED LARGE FILE TRANSFER
 * 
 * The function allocates memory for the extracted portion, which must be freed by the caller.
 * If memory allocation fails or if parameters are invalid, the function returns a FilePortion
 * with NULL data pointer and size of 0.
 * 
 * @param fileContent      Pointer to the complete file content in memory
 * @param totalSize        Total size of the file in bytes
 * @param offset           Byte offset from which to start extraction (absolute mode)
 * @param size             Number of bytes to extract (absolute mode)
 * @param sizePercentage   Percentage of total file to extract (percentage mode)
 * @param offsetPercentage Percentage position to start extraction (percentage mode)
 * 
 * @return FilePortion structure containing the extracted data segment, its size, and offset
 */
FilePortion get_file_portion(const char* fileContent, size_t totalSize, size_t offset, size_t size, double sizePercentage, double offsetPercentage);

/**
 * @brief Processes a directional file transfer by extracting a portion of a file and sending it based on the mode.
 * 
 * This function handles the segmentation and transmission of a portion of a file to a target direction,
 * which can represent a satellite (forward/backward) or ground station (central). It computes the size
 * and offset percentages of the file, extracts the appropriate portion, sets the satellite's ADCS mode 
 * accordingly, and manages visibility conditions if sending to an OGS. It then segments the file portion 
 * into CFDP PDUs and initiates the transfer. A confirmation mechanism ensures successful delivery.
 * 
 * @param direction        Direction of transmission: 0 (central/OGS), 1 (forward), or 2 (backward).
 * @param size             Size of the file segment to transmit, in bytes.
 * @param mode             ADCS mode to align the satellite for transmission.
 * @param connection       Pointer to variable indicating if a connection has been established.
 * @param filename_mem     Filename or identifier for the file being transmitted.
 * @param fileContent      Full content of the file in memory.
 * @param totalSize        Total size of the original file in bytes.
 * @param offset           Offset from the beginning of the file from which to extract the portion.
 * @param info             Pointer to SplittingInfo structure containing satellite and segment info.
 * @param fake_file_size   Total 'virtual' size of the file for calculating relative size percentages.
 * @param vis_start_time   Start time of visibility window for downlink to ground station.
 */
void process_direction(int direction, double size, const uint8 mode, uint8_t *connection, const char *filename_mem, const char* fileContent, size_t totalSize, size_t offset, SplittingInfo* info, double fake_file_size, time_t vis_start_time);

/**
 * @brief Handles the splitting and processing of a large file into segments for different transfer directions.
 *
 * This function divides a file based on the splitting information provided and calls `process_direction`
 * for each direction (Forward, Backward, or Central/OGS). The order of processing depends on the priority
 * direction indicated by the first element of `info->direction`. Offsets are calculated to extract correct
 * portions of the file content. 
 *
 * @param info Pointer to SplittingInfo structure containing segment sizes and direction mapping.
 * @param fileContent Pointer to the complete content of the file to be transmitted.
 * @param fileSize Total size of the input file.
 * @param start_visibility Visibility start time for downlinking to the OGS.
 */
void handle_splitting(SplittingInfo* info, const char *fileContent, const size_t fileSize, time_t start_visibility);

/**
 * @brief Sends a file in segments (PDUs) to a receiving node using a simulated CFDP-like protocol.
 *
 * This function checks memory availability on the receiver side, waits if necessary, and attempts
 * to send each segment with a retry mechanism. After each successful transmission, it simulates
 * a network delay and waits to maintain a fake total duration for the entire file transfer.
 *
 * @param fileContent The content of the file to be sent.
 * @param fileSize The total size of the file in bytes.
 * @param headers Array of PDU file data headers corresponding to each segment.
 * @param contents Array of PDU file data contents for each segment.
 * @param segmentCount The total number of segments into which the file is divided.
 * @param segmentSize The size of each segment.
 * @param connection_establishment Pointer to the current connection establishment flag.
 * @param filename_memory Identifier string for the receiver (used for memory availability checks). If "OGS" it means that the receiver is the GS, so always memory available
 * @param fake_duration The intended fake total transfer duration (used to simulate time spacing in large file transfers).
 * @return int 0 on success, -1 on failure.
 */
int sendIteration(const char *fileContent, size_t fileSize, CF_CFDP_PduFileDataHeader_t *headers, CF_CFDP_PduFileDataContent_t *contents, 
                int segmentCount, int segmentSize, uint8_t *connection_establishment, const char *filename_memory, double fake_duration);

/**
 * @brief Sends a file using CFDP (CCSDS File Delivery Protocol) over an optical inter-satellite link (OISL)
 * 
 * This function handles the complete process of file transmission including:
 * - Memory management and verification on both sender and receiver
 * - File segmentation into Protocol Data Units (PDUs)
 * - Route determination (forward, backward, or to ground station)
 * - Connection establishment with the appropriate target
 * - ADCS (Attitude Determination and Control System) mode adjustment as needed
 * - Reliable transfer with acknowledgments and retry mechanism
 * - Realistic transfer time simulation
 * 
 * @param fileContent Pointer to the content of the file to be sent
 * @param fileSize Size of the file in bytes
 * 
 * @note Each PDU can carry 504 bytes of data
 * @note transfer_time_to_add is used to simulate a large file transfer
 * @note Only the OGS target scenario includes routing, since case 0 and 1 are simple file transfers to neighbor sats
 * @note In the current implementation, for case 0 and 1 the ADCS mode is not changed, i.e. the CMD to change mode must be sent beforehand
 * @note The function handles different routing scenarios based on visibility windows and pass planning
 * @note When downlinking to ground station (OGS), the function will wait for the pass to start before transmitting the data
 */
void sendFile(const char *fileContent, const size_t fileSize);

#endif
