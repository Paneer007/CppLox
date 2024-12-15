# File: segfault_test_fixed.gdb

# Disable pagination and confirmations
set pagination off
set confirm off

# Define the testing loop
define test_until_error
    set $i = 1
    while $i
        echo Running iteration $i...\n
        run
        # Check if the program terminated due to a signal
        if $_isvoid($sig) == 0
            echo Program terminated with signal $sig on iteration $i.\n
            echo Signal description: $_siginfo.si_signame.\n
            break
        else
            echo Iteration $i completed successfully.\n
        end
        set $i = $i + 1
    end
end

# Start the testing loop
test_until_error
