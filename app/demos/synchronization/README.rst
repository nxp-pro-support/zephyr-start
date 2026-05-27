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

From the workspace root::

    west build -p always -b frdm_mcxn947/mcxn947/cpu0 zephyr-start/app/demos/synchronization

The included ``mcxn947.jdebug`` and ``RT1176.jdebug`` Ozone projects assume the
build output is in ``build/zephyr/zephyr.elf`` relative to this app folder.

Sample Output
=============

.. code-block:: console

   thread_a: Hello World from cpu 0 on frdm_mcxn947!
   thread_b: Hello World from cpu 0 on frdm_mcxn947!
   thread_a: Hello World from cpu 0 on frdm_mcxn947!
   thread_b: Hello World from cpu 0 on frdm_mcxn947!

   <repeats endlessly>

Segger SystemView setup
***********************

1. Install Segger SystemView and Ozone:

   - https://www.segger.com/downloads/jlink/#Ozone
   - https://www.segger.com/downloads/systemview/

2. Set the following configurations in the ``prj.conf`` file:

   .. code-block:: none

       CONFIG_STDOUT_CONSOLE=y
       CONFIG_THREAD_NAME=y
       CONFIG_SEGGER_SYSTEMVIEW=y
       CONFIG_USE_SEGGER_RTT=y
       CONFIG_TRACING=y

3. Set up Ozone for your development project.

4. Once Ozone is configured and running the application, run Segger SystemView.
