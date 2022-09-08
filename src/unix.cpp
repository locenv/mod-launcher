#include "launcher.hpp"

#include <locenv/api.hpp>

#include <stdexcept>
#include <system_error>
#include <vector>

#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

process launcher::launch()
{
	// Create a pipe to report child status.
	int pipe[2];

	if (::pipe(pipe) < 0) {
		throw std::system_error(errno, std::system_category());
	}

	if (fcntl(pipe[1], F_SETFD, fcntl(pipe[1], F_GETFD) | FD_CLOEXEC) < 0) {
		auto e = errno;

		close(pipe[0]);
		close(pipe[1]);

		throw std::system_error(e, std::system_category());
	}

	// Fork.
	auto pid = fork();

	switch (pid) {
	case -1: // Error.
		{
			auto e = errno;

			close(pipe[0]);
			close(pipe[1]);

			throw std::system_error(e, std::system_category());
		}
	case 0: // We are in the child.
		{
			close(pipe[0]);

			// Redirect stdin/stdout/stderr to /dev/null.
			auto null = open("/dev/null", O_RDWR);

			if (null < 0 || dup2(null, 0) < 0 || dup2(null, 1) < 0 || dup2(null, 2) < 0) {
				auto e = errno;

				write(pipe[1], &e, sizeof(e));
				exit(1);
			}

			close(null);

			// Setup arguments.
			std::vector<char *> argv;

			argv.push_back(nullptr);

			// Execute.
			if (execv(binary.c_str(), argv.data()) < 0) {
				auto e = errno;

				write(pipe[1], &e, sizeof(e));
				exit(1);
			} else {
				throw 0;
			}
		}
	default: // We are in the parent.
		{
			close(pipe[1]);

			// Wait for child to exec.
			int s;

			for (;;) {
				switch (read(pipe[0], &s, sizeof(s))) {
				case 0: // Exec succeeded.
					close(pipe[0]);

					return process(pid);
				case sizeof(int): // Exec failed.
					waitpid(pid, nullptr, WUNTRACED);
					close(pipe[0]);

					throw std::system_error(s, std::system_category());
				default: // Read error.
					{
						auto e = errno;

						if (e == EINTR) {
							continue;
						}

						kill(pid, SIGKILL);
						waitpid(pid, nullptr, WUNTRACED);
						close(pipe[0]);

						throw std::system_error(e, std::system_category());
					}
				}
			}
		}
	}
}

process::process(pid_t pid) :
	pid(pid)
{
}

process::process(process &&o) :
	pid(o.pid)
{
	o.pid = 0;
}

process::~process()
{
	if (pid) {
		kill(pid, SIGKILL);

		while (waitpid(pid, nullptr, 0) < 0) {
			if (errno != EINTR) {
				// FIXME: How to deal with this case?
				break;
			}
		}
	}
}

int process::wait(locenv::lua l)
{
	if (!pid) {
		return locenv::api->aux_error(l, "process cannot be waited multiple times");
	}

	while (waitpid(pid, nullptr, 0) < 0) {
		if (errno != EINTR) {
			return locenv::api->aux_error(l, "%s", strerror(errno));
		}
	}

	pid = 0;

	return 0;
}
