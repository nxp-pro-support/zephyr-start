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
