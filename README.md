# Process Management Utility
This is a command-line utility written in C for managing processes in a Linux environment. The utility provides various functionalities such as retrieving process information, killing processes, checking process status, and listing process descendants.

# Features
- **Process Information:** Retrieve process information such as PID (Process ID) and PPID (Parent Process ID).
- **Process Killing:** Kill a specific process or its root process.
- **Process Status Checking:** Check if a process is a zombie or paused.
- **Descendant Listing:** List immediate descendants, sibling processes, non-direct descendants, defunct descendants, and grandchildren of a given process.
- **Process Control:** Pause all paused processes in the process tree rooted at a specific process.

## Usage
```bash
./process_util [processID] [rootProcess] [OPTION]
```
## Options
- **-rp:** Kill the specified process.
- **-pr:** Kill the root process.
- **-zs:** Check if the process is a zombie.
- **-xt:** Pause the specified process.
- **-xn:** List non-direct descendants of the specified process.
- **-xd:** List immediate descendants of the specified process.
- **-xs:** List sibling processes of the specified process.
- **-xc:** Continue all paused processes in the process tree.
- **-xz:** List defunct descendants of the specified process.
- **-xg:** List grandchildren of the specified process.
  
## Usage Examples
### Retrieve Process Information
```bash
./process_util <processID> <rootProcess>
```
### Kill a Process
```bash
./process_util <processID> <rootProcess> -rp
```
### Check if a Process is a Zombie
```bash
./process_util <processID> <rootProcess> -zs
```
### List Non-Direct Descendants
```bash
./process_util <processID> <rootProcess> -xn
```
### List Immediate Descendants
```bash
./process_util <processID> <rootProcess> -xd
```
### List Sibling Processes
```bash
./process_util <processID> <rootProcess> -xs
```
### Continue All Paused Processes
```bash
./process_util <processID> <rootProcess> -xc
```
### List Defunct Descendants
```bash
./process_util <processID> <rootProcess> -xz
```
### List Grandchildren
```bash
./process_util <processID> <rootProcess> -xg
```
