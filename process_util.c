#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

// Function to retrieve the Parent Process ID (PPID) of a given process ID
int GetParentPID(int processID) {
    char procPath[50];
    char line[256];
    int parentID = -1;  // Initialize to an invalid value

    // Construct the path to the process status file in the proc filesystem
    snprintf(procPath, sizeof(procPath), "/proc/%d/status", processID);

    // Open the process status file for reading
    FILE *file = fopen(procPath, "r");
    if (!file) {
        perror("Error opening proc file");
        return parentID;
    }

    // Read the PPID (Parent Process ID) from the status file
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "PPid:", 5) == 0) {
            if (sscanf(line + 5, "%d", &parentID) != 1) {
                fprintf(stderr, "Error reading PPID from status file\n");
                parentID = -1;
            }
            break;
        }
    }

    fclose(file);

    return parentID;
}


// Function to check if a given process ID belongs to the process tree rooted at another process
int BelongsToProcessTree(int processID, int rootProcess) {
    int parentID = GetParentPID(processID);

    if (parentID == -1) {
        return 0;  // Unable to get PPID
    }

    if (parentID == rootProcess || BelongsToProcessTree(parentID, rootProcess)) {
        return 1;  // Belongs to the process tree
    } else {
        return 0;  // Does not belong to the process tree
    }
}

// Function to list non-direct descendants of a given process ID
void ListNonDirectDescendants(int processID) {
    char cmd[100];
    int has_nondirect_descendents = 0;
    snprintf(cmd, sizeof(cmd), "pstree -p %d | grep -o '([0-9]\\+)' | grep -o '[0-9]\\+'", processID);

    FILE *fp = popen(cmd, "r");
    if (fp == NULL) {
        perror("Error executing pstree command");
        return;
    }

    printf("Non-Direct Descendants of Process %d:\n", processID);
    char line[50];
    while (fgets(line, sizeof(line), fp) != NULL) {
        int pid = atoi(line);
        int ppid = GetParentPID(pid);
        if (pid != processID && ppid != processID) {
            printf("%d\n", pid);
        }
        has_nondirect_descendents = 1;
    }

    pclose(fp);
    if (!has_nondirect_descendents) {
        printf("No non-direct descendents.\n");
    }
}

// Function to list immediate descendants of a given process ID
void ListImmediateDescendants(int processID) {
    char cmd[100];
    int has_immediate_descendents = 0;
    snprintf(cmd, sizeof(cmd), "pgrep -P %d", processID);

    FILE *fp = popen(cmd, "r");
    if (fp == NULL) {
        perror("Error executing pgrep command");
        return;
    }

    printf("Immediate Descendants of Process %d:\n", processID);
    char line[50];
    while (fgets(line, sizeof(line), fp) != NULL) {
        int pid = atoi(line);
        printf("%d\n", pid);
        has_immediate_descendents = 1;
    }

    pclose(fp);

    if(!has_immediate_descendents) {
        printf("No immediate descendents.\n");
    }
}

// Function to list sibling processes of a given process ID
void ListSiblingProcesses(int processID) {
    int parentID = GetParentPID(processID);
    if (parentID == -1) {
        perror("Unable to get parent process ID");
        return;
    }

    char cmd[100];
    int has_sibling_process = 0;
    snprintf(cmd, sizeof(cmd), "pgrep -P %d", parentID);

    FILE *fp = popen(cmd, "r");
    if (fp == NULL) {
        perror("Error executing pgrep command");
        return;
    }

    printf("Sibling Processes of Process %d:\n", processID);
    char line[50];
    while (fgets(line, sizeof(line), fp) != NULL) {
        int pid = atoi(line);
        if (pid != processID) {
            printf("%d\n", pid);
        }
        has_sibling_process = 1;
    }

    pclose(fp);

    if(!has_sibling_process) {
        printf("No sibling process/es.\n");
    }
}

// Function to list grandchildren of a given process ID
void ListGrandchildren(int processID) {
    char cmd[100];
    int grandchildren_found = 0; // Flag to check if any grandchildren found
    snprintf(cmd, sizeof(cmd), "pgrep -P %d", processID);

    FILE *fp = popen(cmd, "r");
    if (fp == NULL) {
        perror("Error executing pgrep command");
        return;
    }

    printf("Grandchildren of Process %d:\n", processID);
    char line[50];
    while (fgets(line, sizeof(line), fp) != NULL) {
        int parentPID = atoi(line);

        // Use pgrep again to find processes with the parentPID
        snprintf(cmd, sizeof(cmd), "pgrep -P %d", parentPID);
        FILE *childFP = popen(cmd, "r");
        if (childFP == NULL) {
            perror("Error executing pgrep command");
            pclose(fp);
            return;
        }

        // Print the child PIDs
        while (fgets(line, sizeof(line), childFP) != NULL) {
            int childPID = atoi(line);
            printf("%d\n", childPID);
            grandchildren_found = 1;
        }

        pclose(childFP);
    }

    pclose(fp);

    // If no grandchildren found, print message
    if (!grandchildren_found) {
        printf("No Grandchild.\n");
    }
}

// Function to check if a given process ID is paused
int IsPaused(int processID) {
    char procPath[50];
    char line[256];
    char status[50];

    // Construct the path to the process status file in the proc filesystem
    snprintf(procPath, sizeof(procPath), "/proc/%d/status", processID);

    // Open the process status file for reading
    FILE *file = fopen(procPath, "r");
    if (!file) {
        perror("Error opening proc file");
        return 0;  // Unable to open proc file
    }

    // Read the process status from the status file
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "State:", 6) == 0) {
            sscanf(line + 6, "%s", status);
            break;
        }
    }

    fclose(file);

    // Check if the process is paused
    if (strstr(status, "T") != NULL) {
        return 1;  // Paused
    } else {
        return 0;  // Not paused
    }
}

// Function to continue all paused processes in the process tree rooted at a specific process
void ContinueAllPausedProcesses(int rootProcess) {
    char cmd[100];
    snprintf(cmd, sizeof(cmd), "pgrep -P %d", rootProcess);

    FILE *fp = popen(cmd, "r");
    if (fp == NULL) {
        perror("Error executing pgrep command");
        return;
    }

    printf("Continuing paused processes in the process tree rooted at %d:\n", rootProcess);
    char line[50];
    while (fgets(line, sizeof(line), fp) != NULL) {
        int pid = atoi(line);
        // Check if the process is paused
        if (IsPaused(pid)) {
            ContinuePausedProcess(pid);
        }
        else {
            printf("PID %d is not paused.\n", pid);
        }
    }

    pclose(fp);
}


// Function to continue a paused process with a specific process ID
void ContinuePausedProcess(int processID) {
    // Continue the paused process
    if (kill(processID, SIGCONT) == 0) {
        printf("Process with PID %d continued.\n", processID);
    } else {
        perror("Error continuing process");
    }
}

// Function to check if a given process ID represents a zombie process
int IsZombie(int processID) {
    char procPath[50];
    char line[256];
    char status[50];

    // Construct the path to the process status file in the proc filesystem
    snprintf(procPath, sizeof(procPath), "/proc/%d/status", processID);

    // Open the process status file for reading
    FILE *file = fopen(procPath, "r");
    if (!file) {
        perror("Error opening proc file");
        return -1;  // Unable to open proc file
    }

    // Read the process status from the status file
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "State:", 6) == 0) {
            sscanf(line + 6, "%s", status);
            break;
        }
    }

    fclose(file);

    // Check if the process is a zombie
    if (strstr(status, "Z") != NULL) {
        return 1;  // Zombie
    } else {
        return 0;  // Not a zombie
    }
}

// Function to list defunct descendants of a given process ID
void ListDefunctDescendants(int processID) {
    char cmd[100];
    int has_defunct_descendents = 0;
    snprintf(cmd, sizeof(cmd), "pgrep -P %d", processID);

    FILE *fp = popen(cmd, "r");
    if (fp == NULL) {
        perror("Error executing pgrep command");
        return;
    }

    printf("Defunct Descendants of Process %d:\n", processID);
    char line[50];
    while (fgets(line, sizeof(line), fp) != NULL) {
        int pid = atoi(line);
        if (IsZombie(pid)) {
            printf("%d\n", pid);
            has_defunct_descendents = 1;
        }
    }

    pclose(fp);

    if(!has_defunct_descendents) {
        printf("No descendent zombie process/es.\n");
    }
}

int main(int argc, char *argv[]) {
    if (argc != 4 && argc != 3) {
        fprintf(stderr, "Usage: %s [processID] [rootProcess] [OPTION]\n", argv[0]);
        return 1;
    }

    int processID = atoi(argv[1]);
    int rootProcess = atoi(argv[2]);

    // Check if the option is specified
    if (argc == 4) {
        // Check if the process belongs to the process tree
        if (BelongsToProcessTree(processID, rootProcess)) {
            // Process belongs to the process tree
            if (strcmp(argv[3], "-rp") == 0) {
                // Option -rp is specified
                // Kill the process
                if (kill(processID, SIGKILL) == 0) {
                    printf("Process with PID %d killed.\n", processID);
                } else {
                    perror("Error killing process");
                }
            } else if (strcmp(argv[3], "-pr") == 0) {
                // Option -pr is specified
                // Kill the root process
                if (kill(rootProcess, SIGKILL) == 0) {
                    printf("Root Process with ID %d killed. \n", rootProcess);
                } else {
                    perror("Error killing process");
                }
            } else if (strcmp(argv[3], "-zs") == 0) {
                // Option -zs is specified
                // Check if the process is a zombie
                int zombieStatus = IsZombie(processID);
                if (zombieStatus == 1) {
                    printf("Defunct.\n");
                } else if (zombieStatus == 0) {
                    printf("Not Defunct.\n");
                } else {
                    printf("Error checking process %d status.\n", processID);
                }
            } else if (strcmp(argv[3], "-xt") == 0) {
                // Option -xt is specified
                // Pause the process
                if (kill(processID, SIGSTOP) == 0) {
                    printf("Process with PID %d paused.\n", processID);
                } else {
                    perror("Error pausing process");
                }
            } else if (strcmp(argv[3], "-xn") == 0) {
                // Option -xn is specified
                // List non-direct descendants
                ListNonDirectDescendants(processID);
            } else if (strcmp(argv[3], "-xd") == 0) {
                // Option -xd is specified
                // List immediate descendants
                ListImmediateDescendants(processID);
            } else if (strcmp(argv[3], "-xs") == 0) {
                // Option -xs is specified
                // List sibling processes
                ListSiblingProcesses(processID);
            } else if (strcmp(argv[3], "-xc") == 0) {
                // Option -xc is specified
                // Continue all paused processes
                ContinueAllPausedProcesses(rootProcess);
            } else if (strcmp(argv[3], "-xz") == 0) {
                // Option -xz is specified
                // List defunct descendants
                ListDefunctDescendants(processID);
            } else if (strcmp(argv[3], "-xg") == 0) {
                // Option -xg is specified
                // List grandchildren
                ListGrandchildren(processID);
            } else {
                fprintf(stderr, "Invalid option.\n");
                return 1;
            }
        } else {
            // Process does not belong to the process tree
            printf("Process does not belong to the process tree.\n");
        }
    } else {
        // Option is not specified, proceed with regular output
        int parentID = GetParentPID(processID);
        if (parentID != -1 && BelongsToProcessTree(processID, rootProcess)) {
            printf("PID: %d\nPPID: %d\n", processID, parentID);
        } else {
            printf("Does not belong to the process tree\n");
        }
    }
    return 0;
}
