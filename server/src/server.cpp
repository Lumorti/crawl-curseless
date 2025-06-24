#include "./httplib.h"
#include <boost/process.hpp>
#include <mutex>
#include <thread>

// Stuff for the server
httplib::Server svr;

// Whether to output everything or not
bool verbose = false;

// Stuff for the process
namespace bp = boost::process;
std::string fullText = "";
bp::child c;
bp::opstream child_stdin;
bp::ipstream child_stdout;
int numNew = 1;
int nextCommand = 0;
std::vector<std::pair<int, std::string>> commandQueue;
std::mutex readLock;
std::mutex writeLock;
std::mutex outputLock;
std::string args = "";
std::string lastNonTrivialOutput = "";
bool nextGetOutputLast = false;

// C++ entry point
int main(int argc, const char * argv[]) {

    // If any extra argument is given, enable verbose mode
    if (argc > 1) {
        verbose = true;
    }

    // Disable multithreading
    svr.new_task_queue = [] {return new httplib::ThreadPool(8);};

    // Args list
   	args  = " -extra-opt-first monster_item_view_coordinates=true ";
	args += " -extra-opt-first bad_item_prompt=false ";
	args += " -extra-opt-first monster_item_view_features+=cloud ";
	args += " -extra-opt-first monster_item_view_features+=(here) ";
	args += " -extra-opt-first monster_item_view_features+=trap ";
	args += " -extra-opt-first monster_item_view_features+=arch ";
	args += " -extra-opt-first monster_item_view_features+=idol ";
	args += " -extra-opt-first monster_item_view_features+=statue ";
	args += " -extra-opt-first monster_item_view_features+=fountain ";
	args += " -extra-opt-first monster_item_view_features+=translucent ";
	args += " -extra-opt-first monster_item_view_features+=door ";
	args += " -extra-opt-first monster_item_view_features+=gate ";
	args += " -extra-opt-first default_autopickup=false ";
	args += " -extra-opt-first wiz_mode=yes ";
	args += " -extra-opt-first char_set=ascii ";

    // Start the process
    c = bp::child("./crawl.exe" + args, bp::std_out > child_stdout, bp::std_in < child_stdin);

    // On getting the index, we read as much as we can and output
    svr.Get("/get", [](const httplib::Request &, httplib::Response &res) {

        // If we're just outputting the last
        if (nextGetOutputLast && lastNonTrivialOutput.size() > 0) {
            if (verbose) {
                std::cout << "Sending last non-trivial output" << std::endl;
            }
            res.set_content(lastNonTrivialOutput, "text/plain");
            nextGetOutputLast = false;
            return;
        }

        // Get the output from the process
        readLock.lock();
        fullText = "";
        if (c.running()) {
            if (numNew > 0) {
                numNew--;
                char outChar;
                while (child_stdout.get(outChar)) {
                    if (outChar != '\r') {
                        fullText += outChar;
                    }
                    if (fullText.find("===READY===") != std::string::npos) {
                        break;
                    }
                }
            }
        } else {
            fullText = "===CLOSED===";
        }
        size_t startIndex = fullText.find("===START===");
        size_t endIndex = fullText.find("===END===");
        if (startIndex != std::string::npos && endIndex != std::string::npos && endIndex - startIndex > 20) {
            lastNonTrivialOutput = fullText;
        }
        readLock.unlock();

        // Return the full text
        res.set_content(fullText, "text/plain");
        if (verbose) {
            outputLock.lock();
            std::cout << "Client requested a get, sent " << fullText.size() << " chars" << std::endl;
            outputLock.unlock();
        }

        // Give a command if the queue is non-empty
        writeLock.lock();
        if (commandQueue.size() > 0 && commandQueue[0].first == nextCommand) {
            if (verbose) {
                outputLock.lock();
                std::cout << "Running command from queue: " << commandQueue[0].second << " (" << commandQueue[0].first << ")" << std::endl;
                outputLock.unlock();
            }
            child_stdin << commandQueue[0].second << std::endl;
            child_stdin.flush();
            nextCommand++;
            numNew++;
            commandQueue.erase(commandQueue.begin());
        }
        writeLock.unlock();

    });

    // For checking if the server is up
    svr.Get("/", [](const httplib::Request &, httplib::Response &res) {
        if (verbose) {
            std::cout << "Client requested a check" << std::endl;
        }
        res.set_content("===SERVER===", "text/plain");
        nextGetOutputLast = true;
        nextCommand = 0;
    });

    // On command
    svr.Get("/put/:command", [&](const httplib::Request& req, httplib::Response& res) {

        // Get the command
        auto command = req.path_params.at("command");
        if (verbose) {
            outputLock.lock();
            std::cout << "Client sent command: " << command << std::endl;
            outputLock.unlock();
        }

        // Get the number
        std::string commandText = "";
        std::string commandNums = "";
        for (unsigned long int i=0; i<command.size(); i++) {
            if (command[i] >= '0' && command[i] <= '9') {
                commandNums += command[i];
            } else {
                commandText += command[i];
            }
        }

        // Some commands have different text for URL encoding reasons
        command = commandText;
        int commandNum = std::stoi(commandNums);
        if (command == "ctrlX") {
            command = "ctrl-X";
        }

        // Add to the queue
        writeLock.lock();
        bool hasInserted = false;
        for (unsigned long int i=0; i<commandQueue.size(); i++) {
            if (commandNum < commandQueue[i].first) {
                commandQueue.insert(commandQueue.begin()+i, {commandNum, command});
                hasInserted = true;
                break;
            }
        }
        if (!hasInserted) {
            commandQueue.push_back({commandNum, command});
        }
        writeLock.unlock();

        // Generic response
        res.set_content("===DONE===", "text/plain");

    });

    // Start the server
    std::cout << "Starting server on localhost:7777" << std::endl;
    svr.listen("0.0.0.0", 7777);

    // Wait for the process to finish
    c.wait();

    return 0;

}
