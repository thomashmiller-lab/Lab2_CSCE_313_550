
/***********************
Lab 2: Mini Unix Shell
************************/

#include <cerrno>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "Tokenizer.h"

using namespace std;

namespace {

// Part 3: Built-in file commands

bool runBuiltin(const vector<string>& args) {
    if (args.empty()) return true;

    const string& command = args[0];

    // Instruction 11: mycat filename
    // Use open(), read(), write(), and close().
    // Do NOT execute the system cat command.
    if (command == "mycat") {
        if (args.size() != 2) {
            cerr << "Usage: mycat filename" << endl;
            return true;
        }

        int fd = open(args[1].c_str(), O_RDONLY);

        if (fd == -1) {
            perror("mycat");
            return true;
        }

        char buffer[4096];
        ssize_t bytesRead;

        while (true) {
            bytesRead = read(fd, buffer, sizeof(buffer));

            if (bytesRead == -1 && errno == EINTR) continue;

            if (bytesRead == -1) {
                perror("mycat: read");
                break;
            }

            if (bytesRead == 0) break;

            ssize_t totalWritten = 0;

            while (totalWritten < bytesRead) {
                ssize_t bytesWritten = write(STDOUT_FILENO, buffer + totalWritten, static_cast<size_t>(bytesRead - totalWritten));

                if (bytesWritten == -1 && errno == EINTR) continue;

                if (bytesWritten == -1) {
                    perror("mycat: write");
                    close(fd);
                    return true;
                }

                totalWritten += bytesWritten;
            }
        }

        if (close(fd) == -1) perror("mycat: close");

        return true;
    }

    // Instruction 12: myls filename
    // Use stat() to determine file permissions and display
    // the nine permission characters in symbolic notation.
    if (command == "myls") {
        if (args.size() != 2) {
            cerr << "Usage: myls filename" << endl;
            return true;
        }

        struct stat fileInfo {};

        if (stat(args[1].c_str(), &fileInfo) == -1) {
            perror("myls");
            return true;
        }

        string permissions(9, '-');

        if (fileInfo.st_mode & S_IRUSR) permissions[0] = 'r';
        if (fileInfo.st_mode & S_IWUSR) permissions[1] = 'w';
        if (fileInfo.st_mode & S_IXUSR) permissions[2] = 'x';

        if (fileInfo.st_mode & S_IRGRP) permissions[3] = 'r';
        if (fileInfo.st_mode & S_IWGRP) permissions[4] = 'w';
        if (fileInfo.st_mode & S_IXGRP) permissions[5] = 'x';

        if (fileInfo.st_mode & S_IROTH) permissions[6] = 'r';
        if (fileInfo.st_mode & S_IWOTH) permissions[7] = 'w';
        if (fileInfo.st_mode & S_IXOTH) permissions[8] = 'x';

        cout << "Permissions: " << permissions << endl;

        return true;
    }

    // Instruction 13: mychmod MODE filename
    // Convert the octal mode and use chmod().
    // Do NOT execute the system chmod command.
    if (command == "mychmod") {
        if (args.size() != 3) {
            cerr << "Usage: mychmod MODE filename" << endl;
            return true;
        }

        const string& modeText = args[1];

        char* end = nullptr;
        errno = 0;

        long modeValue = strtol(modeText.c_str(), &end, 8);

        if (errno != 0 || end == modeText.c_str() || *end != '\0' || modeText.size() != 3 || modeValue < 0 || modeValue > 0777) {
            cerr << "mychmod: mode must be a three-digit " << "octal value (e.g. 755)" << endl;
            return true;
        }

        if (chmod( args[2].c_str(), static_cast<mode_t>(modeValue)) == -1) perror("mychmod");

        return true;
    }

    // the command is not one of our built-ins somehow
    return false;
}

// Part 2: Input redirection

bool redirectInput(Command& command) {
    if (!command.hasInput()) return true;

    int fd = open(command.in_file.c_str(), O_RDONLY);

    if (fd == -1) {
        perror(command.in_file.c_str());
        return false;
    }

    if (dup2(fd, STDIN_FILENO) == -1) {
        perror("dup2 input");
        close(fd);
        return false;
    }

    close(fd);

    return true;
}




// Part 2: Output and append redirection

bool redirectOutput(Command& command) {
    if (!command.hasOutput()) return true;

    int flags = O_WRONLY | O_CREAT;

    if (command.isAppend()) flags |= O_APPEND;
    else flags |= O_TRUNC;

    int fd = open(command.out_file.c_str(), flags, 0644);

    if (fd == -1) {
        perror(command.out_file.c_str());
        return false;
    }

    if (dup2(fd, STDOUT_FILENO) == -1) {
        perror("dup2 output");
        close(fd);
        return false;
    }

    close(fd);

    return true;
}

// convert command arguments to null-terminated array
// *required by execvp()
vector<char*> makeArgv(const vector<string>& args) {
    
    vector<char*> argv;
    argv.reserve(args.size() + 1);
    for (const string& arg : args) {
        argv.push_back(const_cast<char*>(arg.c_str()));
    }
    argv.push_back(nullptr);
    return argv;
}

// Part 1 and Part 2: Execute commands and pipelines

void executePipeline(Tokenizer& tokenizer) {
    const vector<Command*>& commands = tokenizer.commands;
    const size_t count = commands.size();

    if (count == 0) return;

    // Run a single foreground built-in in the shell process.
    // Save and restore standard descriptors if redirection
    // is used, so the next prompt is unaffected.
    // I hate doing if statments like these grrrr....
    if (count == 1 &&
        !commands[0]->isBackground() &&
        !commands[0]->args.empty() &&
        (commands[0]->args[0] == "mycat" ||
         commands[0]->args[0] == "myls" ||
         commands[0]->args[0] == "mychmod")) {

        int savedIn = -1;
        int savedOut = -1;

        if (commands[0]->hasInput()) {
            savedIn = dup(STDIN_FILENO);

            if (savedIn == -1 || !redirectInput(*commands[0])) {
                if (savedIn != -1) close(savedIn);
                return;
            }
        }

        if (commands[0]->hasOutput()) {
            savedOut = dup(STDOUT_FILENO);

            if (savedOut == -1 || !redirectOutput(*commands[0])) {

                if (savedIn != -1) {
                    dup2(savedIn, STDIN_FILENO);
                    close(savedIn);
                }

                if (savedOut != -1) close(savedOut);

                return;
            }
        }

        runBuiltin(commands[0]->args);

        cout.flush();
        cerr.flush();

        if (savedIn != -1) {
            if (dup2(savedIn, STDIN_FILENO) == -1) perror("restore stdin");
            close(savedIn);
        }

        if (savedOut != -1) {
            if (dup2(savedOut, STDOUT_FILENO) == -1) perror("restore stdout");
            close(savedOut);
        }

        return;
    }

    vector<pid_t> children;

    // read end of the previous pipe
    int previousRead = -1;

    bool background = false;

    // Instruction 4: support an arbitrary number of commands
    for (size_t i = 0; i < count; ++i) {
        int pipeFds[2] = {-1, -1};

        // create a pipe only when another command follows
        if (i + 1 < count && pipe(pipeFds) == -1) {
            perror("pipe");
            if (previousRead != -1) close(previousRead);
            return;
        }

        // Instruction 3: create a child process.
        pid_t pid = fork();

        if (pid == -1) {
            perror("fork");

            if (previousRead != -1) close(previousRead);
            if (pipeFds[0] != -1) close(pipeFds[0]);
            if (pipeFds[1] != -1) close(pipeFds[1]);

            break;
        }

        if (pid == 0) {
            // CHILD PROCESS

            // Connect the previous pipe to standard input
            if (previousRead != -1 && dup2(previousRead, STDIN_FILENO) == -1) {
                perror("dup2 pipe input");
                _exit(1);
            }

            // Connect the next pipe to standard output
            if (pipeFds[1] != -1 && dup2(pipeFds[1], STDOUT_FILENO) == -1) {
                perror("dup2 pipe output");
                _exit(1);
            }

            // Instruction 10: close unused pipe descriptors
            if (previousRead != -1) close(previousRead);
            if (pipeFds[0] != -1) close(pipeFds[0]);
            if (pipeFds[1] != -1) close(pipeFds[1]);

            // Instruction 9: combine pipes and redirection.
            // Explicit file redirection overrides the
            // corresponding pipe endpoint.
            if (!redirectInput(*commands[i]) || !redirectOutput(*commands[i])) _exit(1);

            // Instructions 11-13:
            // Built-ins can also execute inside a pipeline.
            if (runBuiltin(commands[i]->args)) {
                cout.flush();
                cerr.flush();
                _exit(0);
            }

            // Instruction 3: execute a normal Linux command.
            vector<char*> argv = makeArgv(commands[i]->args);

            if (argv.size() <= 1 || argv[0] == nullptr || argv[0][0] == '\0') _exit(0);

            execvp(argv[0], argv.data());

            // execvp() returns only if it fails.
            perror(commands[i]->args[0].c_str());

            _exit(127);
        }


        // PARENT PROCESS

        children.push_back(pid);

        background = background || commands[i]->isBackground();

        // Instruction 10: close parent copies that are
        // no longer needed.
        if (previousRead != -1) close(previousRead);

        if (pipeFds[1] != -1) close(pipeFds[1]);

        // Keep the new pipe's read end for the next command.
        previousRead = pipeFds[0];
    }

    // Close any remaining pipe read end in the parent.
    if (previousRead != -1) {
        close(previousRead);
    }

    // Instruction 5: wait for all foreground children.
    // Instruction 16: manage child processes and resources.
    if (!background) {
        for (pid_t child : children) {
            int status;

            while (waitpid(child, &status, 0) == -1) {
                if (errno == EINTR) continue;
                perror("waitpid");
                break;
            }
        }
    }
}

} // namespace

int main() {

    // TODO:
    // Implement your mini Unix shell.
    //
    // Your shell must support the following functionality:
    //
    // -------------------------------------------------------
    // Part 1: Processes and Pipes
    // -------------------------------------------------------
    //
    // 1. Repeatedly prompt the user for commands.
    //
    // 2. Exit when the user enters:
    //
    //      exit
    //
    // 3. Execute normal Linux commands using fork() and execvp().
    //
    // 4. Execute an arbitrary number of piped commands.
    //
    //      command1 | command2
    //
    //      command1 | command2 | command3 | ...
    //
    // 5. Wait for all foreground child processes before
    //    displaying the next prompt.
    //
    //
    // -------------------------------------------------------
    // Part 2: File I/O and I/O Redirection
    // -------------------------------------------------------
    //
    // 6. Support input redirection:
    //
    //      command < file
    //
    // 7. Support output redirection:
    //
    //      command > file
    //
    // 8. Support append redirection:
    //
    //      command >> file
    //
    // 9. Correctly combine pipes and redirection.
    //
    //      command1 < input.txt | command2 | command3 > output.txt
    //
    // 10. Correctly close all unused file descriptors.
    //
    //
    // -------------------------------------------------------
    // Part 3: File Operations and Permissions
    // -------------------------------------------------------
    //
    // 11. Implement the built-in command:
    //
    //      mycat filename
    //
    //     mycat must use open(), read(), write(), and close().
    //     Do NOT execute the system cat command.
    //
    // 12. Implement the built-in command:
    //
    //      myls filename
    //
    //     myls must use stat() to determine the file's
    //     permissions and display them in symbolic notation.
    //
    //     Example:
    //
    //      Permissions: rw-r--r--
    //
    // 13. Implement the built-in command:
    //
    //      mychmod MODE filename
    //
    //     Example:
    //
    //      mychmod 755 script.sh
    //
    //     mychmod must use chmod().
    //     Do NOT execute the system chmod command.
    //
    //
    // -------------------------------------------------------
    // General Requirements
    // -------------------------------------------------------
    //
    // 14. Handle common errors without crashing the shell.
    //
    // 15. Do NOT use system() to execute commands.
    //
    // 16. Properly manage child processes and file descriptors.
    //
    // See the Lab 2 handout for detailed requirements.

    // Instructions 1-2: repeatedly prompt until exit or EOF.
    string line;

    while (true) {
        cout << "myshell> " << flush;

        if (!getline(cin, line)) break;

        if (line == "exit") break;

        if (line.empty()) continue;

        Tokenizer tokenizer(line);

        if (tokenizer.hasError()) continue;

        // Reject empty commands in a pipeline.
        bool hasEmptyCommand = false;

        for (const Command* command : tokenizer.commands) {
            if (command->args.empty() || command->args[0].empty()) {
                hasEmptyCommand = true;
                break;
            }
        }

        if (hasEmptyCommand) {
            cerr << "Invalid command: empty command in pipeline" << endl;
            continue;
        }

        executePipeline(tokenizer);
    }

    return 0;
}
