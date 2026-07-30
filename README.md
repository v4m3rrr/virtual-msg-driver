# Virtual Message Device Driver (vmdd)
A Linux kernel character device that exposes in-memory FIFO message queues to user space.
## Features
  - Creates up to 8 independent character devices (`/dev/vmdd_0`, `/dev/vmdd_1`, ...), each backed by its own `kfifo` queue.
  - Blocking and non-blocking IO.
  - `poll()` / `select()` support.
  - `ioctl()` command to query the number of bytes currently queued.
  - Per-device sysfs attribute `count` showing the current queue length.
  - Configurable number of devices via a module parameter.
## Build
### Step 1: Clone the repository
1. Open a command-line interface.
2. Navigate to the directory to clone the project:
   ```bash
   cd /path/to/my/directory
   ```
3. Clone the project:  
   ```bash
   git clone https://github.com/v4m3rrr/virtual-msg-driver.git
   ```
### Step 2: Run make
  - Navigate to the cloned directory:
    ```bash
    cd virtual-msg-driver
    ```
  - Run the following command:
    ```bash
    make
    ```
  - To clean build artifacts
    ```bash
    make clean
    ```
## Loading module
  ```bash
  sudo insmod vmdd.ko
  ```
  By default driver creates 3 device drivers. To change this, pass the `vmdd_cdev_count` parameter:
  ```bash
  sudo insmod vmdd.ko vmdd_cdev_count=<number_of_devices>
  ```
  To unload the module:
   ```bash
  sudo rmmod vmdd
  ```
