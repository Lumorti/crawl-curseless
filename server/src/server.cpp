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
int numNew = 1;

// C++ entry point
int main(int argc, const char * argv[]) {

    // Disable multithreading
    svr.new_task_queue = [] {return new httplib::ThreadPool(2);};

    // Start the process
    c = bp::child("./crawl.exe", bp::std_out > child_stdout, bp::std_in < child_stdin);

    // On getting the index, we read as much as we can and output
    svr.Get("/get", [](const httplib::Request &, httplib::Response &res) {
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
        res.set_content(fullText, "text/plain");
        std::cout << "Client requested a get, sent " << fullText.size() << " chars" << std::endl;
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
        numNew++;
        res.set_content("===DONE===", "text/plain");
    });

    // Start the server
    std::cout << "Starting server on localhost:7777" << std::endl;
    svr.listen("0.0.0.0", 7777);

    // Wait for the process to finish
    c.wait();

    return 0;

}
