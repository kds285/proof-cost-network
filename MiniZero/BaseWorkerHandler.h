#pragma once

#include <deque>
#include <set>
#include <string>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/atomic.hpp>
#include <boost/make_shared.hpp>

using boost::asio::ip::tcp;

class BaseWorkerStatus : public boost::enable_shared_from_this<BaseWorkerStatus> {
protected:
	boost::shared_ptr<boost::asio::io_service::strand> m_strand;
	boost::shared_ptr<tcp::socket> m_socket;
	boost::asio::streambuf m_buffer;
	boost::atomic<bool> m_bIsEnd;
	std::deque<std::string> m_msg_queue;

public:
	BaseWorkerStatus(boost::shared_ptr<tcp::socket> socket)
		: m_strand(new boost::asio::io_service::strand(socket->get_io_service()))
		, m_socket(socket)
		, m_bIsEnd(false)
	{
	}

	~BaseWorkerStatus()
	{
	}

	void write(std::string msg)
	{
		if (m_bIsEnd) { return; }
		if (msg.empty()) { return; }
		if (msg[msg.length() - 1] != '\n') msg += '\n';

		m_strand->dispatch(boost::bind(&BaseWorkerStatus::do_write, shared_from_this(), msg));
	}

	void do_write(const std::string msg)
	{
		m_msg_queue.push_back(msg);
		if (m_msg_queue.size() > 1) {
			// outstanding async_write, will call handle_write later
			return;
		} else {
			// all async_write is completed, safe to start writing
			write_next();
		}
	}

	void write_next()
	{
		const std::string& cmd_sending = m_msg_queue.front();

		boost::asio::async_write(*m_socket,
			boost::asio::buffer(cmd_sending),
			m_strand->wrap(boost::bind(&BaseWorkerStatus::handle_write, shared_from_this(), boost::asio::placeholders::error)));
	}

	void handle_write(const boost::system::error_code& error)
	{
		m_msg_queue.pop_front();

		if (error) {
			std::cerr << "Could not write" << std::endl;
			do_close();
			return;
		}

		if (!m_msg_queue.empty()) {
			write_next();
		}
	}

	void start_read()
	{
		boost::asio::async_read_until(*m_socket,
			m_buffer, '\n',
			boost::bind(&BaseWorkerStatus::handle_read, shared_from_this(),
				boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
	}

	virtual void do_close()
	{
		if (!m_bIsEnd) {
			m_bIsEnd = true;
			m_socket->close();
		}
	}

	void handle_read(const boost::system::error_code& error, size_t bytes_read)
	{
		if (error) {
			do_close();
			return;
		}

		std::istream is(&m_buffer);
		std::string line;
		std::getline(is, line);
		handle_msg(line);

		start_read();
	}

	virtual void handle_msg(const std::string msg) = 0;
	bool isEnd() { return m_bIsEnd; }
};


template<class _WorkerStatus>
class BaseWorkerHandler {
protected:
	boost::asio::io_service m_io_service;
	boost::shared_ptr<tcp::socket> m_nextSocket;
	tcp::acceptor m_acceptor;
	boost::thread_group m_workerThreadPool;
	boost::asio::io_service::work m_work;

	std::set<boost::shared_ptr<_WorkerStatus> > m_vWorkers;
	boost::mutex m_workerMutex;

public:
	BaseWorkerHandler(int port)
		: m_acceptor(m_io_service)
		, m_work(m_io_service)
	{
		tcp::endpoint endpoint(tcp::v4(), port);
		m_acceptor.open(endpoint.protocol());
		boost::asio::socket_base::reuse_address option(true);
		m_acceptor.set_option(option);
		m_acceptor.bind(endpoint);
		m_acceptor.listen();

		const int numZeroWorkerHandlerThreads = 1;
		for (int i = 0; i < numZeroWorkerHandlerThreads; ++i) {
			m_workerThreadPool.create_thread(boost::bind(&BaseWorkerHandler::run, this));
		}
	}

	~BaseWorkerHandler()
	{
		m_workerThreadPool.join_all();
	}

	void run()
	{
		m_io_service.run();
	}

	void stop()
	{
		m_io_service.stop();
	}

	void start_accept()
	{
		m_nextSocket = boost::make_shared<tcp::socket>(m_io_service);
		m_acceptor.async_accept(*m_nextSocket, boost::bind(&BaseWorkerHandler::handle_accept, this, boost::asio::placeholders::error));
	}

private:
	void handle_accept(const boost::system::error_code& error)
	{
		if (!error) {
			boost::lock_guard<boost::mutex> lock(m_workerMutex);

			// delete finished worker in set
			for (auto workerIter = m_vWorkers.begin(), last = m_vWorkers.end(); workerIter != last; ) {
				if ((*workerIter)->isEnd()) {
					workerIter = m_vWorkers.erase(workerIter);
					//std::cerr << "handle_accept: removed a worker." << std::endl;
				} else {
					++workerIter;
				}
			}

			// create a worker
			boost::shared_ptr<_WorkerStatus> worker = handle_accept_new_worker(m_nextSocket);
			worker->start_read();
			m_vWorkers.insert(worker);
		}
		start_accept();
	}

protected:
	// return new worker status shared_ptr
	virtual boost::shared_ptr<_WorkerStatus> handle_accept_new_worker(boost::shared_ptr<tcp::socket> socket) = 0;
};
