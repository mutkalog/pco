// #include "linuxsandbox.h"


// #include <optional>
// #include <sys/socket.h>
// #include <unistd.h>
// #include <filesystem>
// #include <sys/mount.h>
// #include <sched.h>
// #include <errno.h>
// #include <sys/syscall.h>

// #include <iostream>
// #include <sys/stat.h>
// #include <sys/wait.h>
// #include <spawn.h>
// #include <fstream>
// #include <elf.h>

// #include "../updatecontext.h"


// namespace fs = std::filesystem;

// void LinuxSandbox::prepare(UpdateContext &context)
// {
//     createRootfs(context);
//     copyDependencies(context);

//     if (socketpair(AF_UNIX, SOCK_STREAM, 0, socketsFds_) != 0)
//         throw std::system_error(std::error_code(errno, std::generic_category()),
//                                 "soketpair() failed");

//     pid_t pid = fork();
//     if (pid == -1)
//         throw std::system_error(std::error_code(errno, std::generic_category()),
//                                 "fork() failed");

//     if (pid == 0)
//     {
//         buildSandbox();

//         Message msg = socketRead(CHILD);
//         if (msg.type != COMMAND)
//             throw std::runtime_error("Wrong message received. "
//                                      "Must be COMMAND, received "
//                                      + std::to_string(msg.type));

//         std::vector<pid_t> launchedPids;

//         switch (msg.command)
//         {
//         case RUN:
//             for (size_t i = 0; i != context.manifest.files.size(); ++i)
//             {
//                 auto& files = context.manifest.files;

//                 if (files[i].isExecutable == false)
//                     continue;

//                 fs::path    path        = files[i].installPath;
//                 std::string programName = path.filename();

//                 if (programName.empty())
//                 {
//                     std::memset(&msg, 0, sizeof(msg));
//                     msg.type = STATUS_CODE;
//                     msg.status = FAIL;
//                     socketReport(CHILD, msg, "wrong filename failed");
//                 }

//                 // char* argv[2] = {const_cast<char*>(programName.c_str()), nullptr};

//                 pid_t childPid = 0;
//                 auto args = ArtifactManifest::getFileArgs(files[i]);
//                 if (posix_spawn(&childPid, path.c_str(),
//                                 nullptr, nullptr, args.data(), nullptr) != 0)
//                 {
//                     std::memset(&msg, 0, sizeof(msg));
//                     msg.type = STATUS_CODE;
//                     msg.status = FAIL;
//                     socketReport(CHILD, msg, std::string("Cannot launch ") + path.string());
//                     break;
//                 }

//                  std::memset(&msg, 0, sizeof(msg));
//                  msg.type = PROCESS_ID;
//                  msg.processInfo.pid = childPid;
//                  socketReport(CHILD, msg, std::string("PASSED CHILD PID ") +
//                                   std::to_string(childPid) + " from container");

//                  launchedPids.push_back(childPid);

//                  msg = socketRead(CHILD);
//                  if (msg.type != COMMAND)
//                      throw std::runtime_error("Wrong message received."
//                                               " Must be COMMAND,"
//                                               " received " +
//                                               std::to_string(msg.type));

//                  if (msg.command != RUN_NEXT)
//                  {
//                      std::cout << "Received not RUN_NEXT command,"
//                                   " aborting launching..."
//                                << std::endl;
//                      break;
//                 }
//             }
//             break;

//             int status;
//             int rc;
//             while (launchedPids.empty() == false)
//             {
//                 pid_t wp = waitpid(-1, &status, 0);
//                 if (wp == -1)
//                 {
//                     std::memset(&msg, 0, sizeof(msg));
//                     msg.type   = STATUS_CODE;
//                     msg.status = FAIL;
//                     socketReport(CHILD, msg, std::string("waitpid(-1) failed"));
//                     exit(EXIT_FAILURE);
//                 }

//                 auto it = std::find(launchedPids.begin(), launchedPids.end(), wp);
//                 if (it != launchedPids.end())
//                 {
//                     std::memset(&msg, 0, sizeof(msg));
//                     msg.type = PROCESS_EXIT_STATUS;
//                     msg.processInfo.exitCode = status;
//                     msg.processInfo.pid = wp;

//                     socketReport(CHILD, msg, std::string("Process ") + std::to_string(*it)
//                                                 + " exited with status " + std::to_string(status));

//                     launchedPids.erase(it);
//                 }
//                 else
//                 {
//                     __builtin_unreachable();
//                 }
//             }

//             std::cout << "Container goes into dead state" << std::endl;

//             break;
//         case ABORT:
//             std::cout << "Abort command received."
//                          " Aborting" << std::endl;
//             exit(0);
//             break;
//         }

//         exit(EXIT_SUCCESS);
//     }
//     else
//     {
//         pid_ = pid;
//         std::cout << "CONTAINER PID = " << pid << std::endl;
//     }
// }

// void LinuxSandbox::launch(UpdateContext &context)
// {
//     std::cout << "SANDBOX LAUNCH pid=" << getpid()
//           << " tid=" << std::this_thread::get_id() << std::endl;

//     Message msg = socketRead(PARENT);
//     if (msg.type != STATUS_CODE)
//     {
//         throw std::runtime_error("Wrong message received."
//                                  " Must be STATUS_CODE, "
//                                  "received " + std::to_string(msg.type));
//     }

//     switch (msg.status)
//     {
//     case OK:
//         std::memset(&msg, 0, sizeof(msg));
//         msg.type = COMMAND;
//         msg.command = RUN;

//         socketReport(PARENT, msg, "Launch command sent");
//         break;
//     default:
//         throw std::runtime_error("Deployment failed");
//     }

//     const auto& files = context.manifest.files;

//     for (size_t i = 0; i != files.size(); ++i)
//     {
//         if (files[i].isExecutable == true)
//         {
//             auto receivedMsg = socketRead(PARENT);
//             if (receivedMsg.type == PROCESS_ID)
//             {
//                 context.containeredProcesees.push_back(receivedMsg.processInfo.pid);

//                 std::memset(&msg, 0, sizeof(msg));
//                 msg.type    = COMMAND;
//                 msg.command = RUN_NEXT;

//                 socketReport(PARENT, msg, "Launch next process command sent");
//             }
//             else if (receivedMsg.type == STATUS_CODE)
//             {
//                 throw std::runtime_error("Failed to start app");
//             }
//             else
//             {
//                  throw std::runtime_error("Wrong message received. "
//                                          "Must be PROCESS_ID or STATUS_CODE, "
//                                          "received " + std::to_string(msg.type));
//             }
//         }
//     }
// }

// void LinuxSandbox::cleanup(UpdateContext &context)
// {
//     if (busyResources_.sandbox == 1)
//     {
//         fs::remove_all(path_);
//         std::cout << "LinuxSandbox: sandbox removed" << std::endl;
//     }

//     busyResources_.sandbox = 0;
// }

// std::pair<pid_t, int> LinuxSandbox::getReturnCode()
// {
//     Message msg = socketRead(PARENT);
//     if (msg.type == STATUS_CODE && msg.status == FAIL)
//          throw std::runtime_error("Cannot get process rc");
//     else if (msg.type != PROCESS_EXIT_STATUS)
//          throw std::runtime_error("Wrong message received. Must be PROCESS_ID, received " + std::to_string(msg.type));

//     return {msg.processInfo.pid, msg.processInfo.exitCode};
// }

// void LinuxSandbox::buildSandbox()
// {
//     int rc = unshare(CLONE_NEWNS | CLONE_NEWPID);
//     Message message{};
//     message.type   = STATUS_CODE;
//     message.status = FAIL;

//     if (rc != 0)
//         socketReport(CHILD, message, "unshare() failed");

//     rc = mount(nullptr, "/", nullptr, MS_REC | MS_PRIVATE, nullptr);
//     if (rc != 0)
//         socketReport(CHILD, message, "mount() failed");

//     rc = mount(path_.c_str(), path_.c_str(),
//                nullptr, MS_BIND | MS_REC, nullptr);
//     if (rc != 0)
//         socketReport(CHILD, message, "mount() failed");

//     rc = syscall(SYS_pivot_root, path_.c_str(),
//                  fs::path(path_ / "oldroot").c_str());
//     if (rc != 0)
//         socketReport(CHILD, message, "pivot_root() failed");

//     rc = mount("proc", "/proc", "proc", 0, nullptr);
//     if (rc != 0)
//         socketReport(CHILD, message, "/proc mount() failed");

//     rc = mount("/dev", "/dev", nullptr, MS_BIND | MS_REC, nullptr);
//     if (rc != 0)
//         socketReport(CHILD, message, "/dev mount() failed");

//     rc = mount("sysfs", "/sys", "sysfs", 0, nullptr);
//     if (rc != 0)
//         socketReport(CHILD, message, "/sys mount() failed");

//     rc = umount2("/oldroot", MNT_DETACH);
//     if (rc != 0)
//         socketReport(CHILD, message, "umount() failed");

//     try
//     {
//         fs::remove_all("/oldroot");
//     }
//     catch (const fs::filesystem_error &ex)
//     {
//         socketReport(CHILD, message, "remove_all(/oldroot) failed");
//     }

//     message.status = OK;
//     socketReport(CHILD, message, "Environment has been built");
// }

// void LinuxSandbox::copyDependencies(const UpdateContext &context)
// {
//     ArtifactManifest manifest = context.manifest;

//     std::set<std::string> requiredLibs;

//     for (const auto& file : context.manifest.files)
//     {
//         if (file.isExecutable == false)
//             continue;

//         // fs::path path   = context.sb->getPath() / fs::path(file.installPath).relative_path();
//         auto     interp = getInterpretator(path);

//         if (interp.empty())
//             continue; // static-linked executable

//         requiredLibs.insert(interp);

//         std::string cmd = interp + " --list " + path.string() + " 2>/dev/null";
//         std::array<char, 512> buf;
//         std::vector<std::string> result;
//         std::unique_ptr<FILE, int (*) (FILE*)> pipe(popen(cmd.c_str(), "r"),
//                                                       pclose);

//         if (!pipe)
//             throw std::runtime_error("popen failed");

//         while (fgets(buf.data(), buf.size(), pipe.get()))
//         {
//             std::string line(buf.data());

//             auto libOpt = parseInterpretatorLine(line);

//             if (libOpt.has_value() == true)
//                 auto [it, inserted] = requiredLibs.insert(*libOpt);
//         }
//     }

//     for (const auto& lib : requiredLibs)
//     {
//         fs::path targetFile = path_ / fs::path(lib).parent_path().relative_path();
//         fs::create_directories(targetFile);
//         fs::copy(lib, targetFile);
//     }

//     /// @todo потом убрать
//     fs::copy("/bin/busybox", path_ / "bin");
// }

// #ifdef p_type
// #undef p_type
// #endif

// std::string LinuxSandbox::getInterpretator(const std::filesystem::__cxx11::path &path)
// {
//     std::ifstream f(path, std::ios::binary);
//     if (!f)
//         throw std::runtime_error("Cannot open file");

//     Elf64_Ehdr eh;
//     f.read(reinterpret_cast<char*>(&eh), sizeof(eh));

//     if (memcmp(eh.e_ident, ELFMAG, SELFMAG) != 0)
//         throw std::runtime_error("not ELF");

//     f.seekg(eh.e_phoff);

//     for (int i = 0; i < eh.e_phnum; ++i)
//     {
//         Elf64_Phdr ph;
//         f.read(reinterpret_cast<char*>(&ph), sizeof(ph));
//         if (ph.p_type == PT_INTERP)
//         {
//             std::vector<char> buf(ph.p_filesz);
//             auto cur = f.tellg();
//             f.seekg(ph.p_offset);
//             f.read(buf.data(), buf.size());
//             f.seekg(cur);
//             return std::string(buf.data());
//         }
//     }

//     return ""; // статически слинкованная программа
// }

// std::optional<std::string>
// LinuxSandbox::parseInterpretatorLine(const std::string& line)
// {
//     const std::regex pattern(R"(^\s*\S+\s+=>\s+(?:(\/\S+)|(not found)))");

//     std::smatch match;

//     if (std::regex_search(line, match, pattern) == false)
//     {
//         return std::nullopt;
//     }

//     if (match[1].matched == true)
//     {
//         return std::make_optional(match[1].str());
//     }
//     else if (match[2].matched == true)
//     {
//         throw std::runtime_error("Cannot find lib " + line);
//     }

//     return std::nullopt;
// }

// void LinuxSandbox::createRootfs(const UpdateContext &context)
// {
//     std::vector<fs::path> rootfs{"bin", "sys", "lib", "lib64", "dev", "proc", "tmp", "oldroot"};

//     for (const auto& dir : rootfs)
//     {
//         fs::path directory = path_ / dir;

//         if (fs::create_directory(directory) == false)
//             throw std::runtime_error("Cannot create rootfs directory \"" + directory.string() + "\"");

//         busyResources_.sandbox = 1;
//     }
// }

// void LinuxSandbox::socketReport(int sockFd, Message msg, const std::string &logMessage)
// {
//     std::cout << "SOCKET REPORT IN " << getpid() << std::endl;
//     std::cout << logMessage << std::endl;
//     raw_message_t buf;
//     std::memcpy(buf.data(), &msg, sizeof(msg));
//     if (write(socketsFds_[sockFd], buf.data(), buf.size()) == -1)
//     {
//         throw std::system_error(std::error_code(errno, std::generic_category()),
//                                 "write() failed");
//     }

//     ///@todo вынести из фукнции
//     if (msg.type == STATUS_CODE && msg.status == FAIL)
//         exit(EXIT_FAILURE);
// }

// Message LinuxSandbox::socketRead(int sockFd)
// {
//     // std::cout << "SOCKET READ IN " << getpid() << std::endl;

//     raw_message_t buf{};
//     if (read(socketsFds_[sockFd], buf.data(), buf.size()) <= 0)
//         throw std::system_error(std::error_code(errno, std::generic_category()),
//                                 " read() failed");

//     Message msg{};
//     std::memcpy(&msg, buf.data(), buf.size());
//     return msg;
// }
