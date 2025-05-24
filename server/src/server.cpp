#include "./httplib.h"
#include <boost/process.hpp>

// Stuff for the server
httplib::Server svr;

// Stuff for the process
namespace bp = boost::process;
std::string fullText = "";
bp::child c;
bp::opstream child_stdin;
bp::ipstream child_stdout;
bool hasBeenInput = true;

// C++ entry point
int main(int argc, const char * argv[]) {

    // Disable multithreading
    svr.new_task_queue = [] { return new httplib::ThreadPool(1); };

    // Start the process
    c = bp::child("./crawl.exe", bp::std_out > child_stdout, bp::std_in < child_stdin);

    // On getting the index, we read as much as we can and output
    svr.Get("/get", [](const httplib::Request &, httplib::Response &res) {
        std::cout << "Client requested a get" << std::endl;
        fullText = "";
        if (c.running()) {
            if (hasBeenInput) {
                std::string line;
                while (std::getline(child_stdout, line)) {
                    for (char c : line) {
                        if (c != '\r' && c != '\n') {
                            fullText += c;
                        }
                    }
                    fullText += '\n';
                    if (line.size() == 0 || line.find("===READY===") != std::string::npos) {
                        break;
                    }
                }
                hasBeenInput = false;
            }
        } else {
            fullText = "===CLOSED===";
        }
        res.set_content(fullText, "text/plain");
    });

    // For checking if the server is up
    svr.Get("/", [](const httplib::Request &, httplib::Response &res) {
        std::cout << "Client requested a check" << std::endl;
        res.set_content("===SERVER===", "text/plain");
    });

    // On command
    svr.Get("/put/:command", [&](const httplib::Request& req, httplib::Response& res) {
        auto command = req.path_params.at("command");
        std::cout << "Client sent command: " << command << std::endl;
        if (command == "ctrlX") {
            command = "ctrl-X";
        }
        child_stdin << command << std::endl;
        child_stdin.flush();
        hasBeenInput = true;
        res.set_content("===DONE===", "text/plain");
    });

    // Start the server
    std::cout << "Starting server on localhost:7777" << std::endl;
    svr.listen("0.0.0.0", 7777);

    // Wait for the process to finish
    c.wait();

    return 0;

}
