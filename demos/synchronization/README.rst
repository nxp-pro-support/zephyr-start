<<<<<<< HEAD
Segger SystemView setup
========================

1. Install Segger SystemView and Ozone 
    - https://www.segger.com/downloads/jlink/#Ozone
    - https://www.segger.com/downloads/systemview/

2. Set the following configurations in the “proj.conf” file:

.. code-block:: none

    CONFIG_STDOUT_CONSOLE=y
    CONFIG_THREAD_NAME=y
    CONFIG_SEGGER_SYSTEMVIEW=y
    CONFIG_USE_SEGGER_RTT=y
    CONFIG_TRACING=y

3. Set up Ozone for your development project.

4. Once Ozone is configured and running the application, run Segger SystemView.
=======
.. zephyr:code-sample:: synchronization
   :name: Basic Synchronization
   :relevant-api: thread_apis semaphore_apis

   Manipulate basic kernel synchronization primitives.

Overview
********

A simple application that demonstrates basic sanity of the kernel.
Two threads (A and B) take turns printing a greeting message to the console,
and use sleep requests and semaphores to control the rate at which messages
are generated. This demonstrates that kernel scheduling, communication,
and timing are operating correctly.

Building and Running
********************

This project outputs to the console.  It can be built and executed
on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/synchronization
   :host-os: unix
   :board: qemu_x86
   :goals: run
   :compact:

Sample Output
=============

.. code-block:: console

   threadA: Hello World!
   threadB: Hello World!
   threadA: Hello World!
   threadB: Hello World!
   threadA: Hello World!
   threadB: Hello World!
   threadA: Hello World!
   threadB: Hello World!
   threadA: Hello World!
   threadB: Hello World!

   <repeats endlessly>

Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.
>>>>>>> fad8e7f307a729ccbcaa3d8aeececd0ff80c1f01
