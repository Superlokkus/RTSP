#include <rtsp_methods.hpp>

#include <algorithm>
#include <random>
#include <fstream>

#include <rtsp_headers.hpp>
#include <rtp_endsystem.hpp>

rtsp::response rtsp::methods::common_response_sekeleton(const rtsp::rtsp_server_session &session,
                                                        const rtsp::internal_request &request) {
    return rtsp::response{common_rtsp_major_version, common_rtsp_minor_version,
                          200, "OK", {
                                  {"CSeq", request.second.at("cseq")},
                                  {"Session", session.identifier()},
                          }};
}

auto get_acceptable_transport(const rtsp::headers::transport &requested)
-> boost::optional<rtsp::headers::transport::transport_spec> {
    thread_local std::default_random_engine re{};
    using namespace rtsp;
    boost::optional<rtsp::headers::transport::transport_spec> choosen{};

    auto requested_transport_spec = std::find_if(requested.specifications.cbegin(),
                                                 requested.specifications.cend(),
                                                 [](const headers::transport::transport_spec &value) {
                                                     return value.transport_protocol == "RTP" &&
                                                            value.profile == "AVP" &&
                                                            value.lower_transport.value_or("UDP") == "UDP" &&
                                                            value.parameters.size() > 0 &&
                                                            value.parameters.at(0).which() == 4 &&
                                                            boost::get<string>(value.parameters.at(0)) == "unicast";
                                                 });
    if (requested_transport_spec == requested.specifications.cend())
        return choosen;

    rtsp::headers::transport::transport_spec choosen_spec{"RTP", "AVP", boost::make_optional<string>("UDP")};

    auto parameter_begin = requested_transport_spec->parameters.cbegin();
    auto parameter_end = requested_transport_spec->parameters.cend();

    choosen_spec.parameters.push_back(rtsp::string{"unicast"});

    auto parameter = std::find_if(parameter_begin, parameter_end,
                                  [](const auto &value) {
                                      return value.which() == 1 &&
                                             boost::get<headers::transport::transport_spec::port>(value).type ==
                                             headers::transport::transport_spec::port::port_type::client;
                                  });
    if (parameter == parameter_end) {
        std::uniform_int_distribution<headers::transport::transport_spec::port_number> ud{49152u, 65525u};
        headers::transport::transport_spec::port client_port{
                headers::transport::transport_spec::port::port_type::client,
                ud(re)};
        choosen_spec.parameters.push_back(client_port);
    } else {
        const auto &suggest_port = boost::get<headers::transport::transport_spec::port>(*parameter);
        headers::transport::transport_spec::port client_port{
                headers::transport::transport_spec::port::port_type::client,
                headers::transport::transport_spec::port_number{}};
        if (suggest_port.port_numbers.which() == 0) {
            client_port.port_numbers = suggest_port.port_numbers;
        } else {
            client_port.port_numbers = boost::get<headers::transport::transport_spec::port_range>
                    (suggest_port.port_numbers).first;
        }

        choosen_spec.parameters.push_back(client_port);
    }


    parameter = std::find_if(parameter_begin, parameter_end,
                             [](const auto &value) {
                                 return value.which() == 2 &&
                                        boost::get<headers::transport::transport_spec::ssrc>(value) != 0u;
                             });
    if (parameter == parameter_end) {
        std::uniform_int_distribution<headers::transport::transport_spec::ssrc> ud{};
        choosen_spec.parameters.push_back(headers::transport::transport_spec::ssrc{ud(re)});
    } else {
        choosen_spec.parameters.push_back(*parameter);
    }

    parameter = std::find_if(parameter_begin, parameter_end,
                             [](const auto &value) {
                                 return value.which() == 1 &&
                                        boost::get<headers::transport::transport_spec::port>(value).type ==
                                        headers::transport::transport_spec::port::port_type::server;
                             });
    if (parameter == parameter_end) {
        const auto &client_port = boost::get<headers::transport::transport_spec::port>(choosen_spec.parameters.at(1));
        headers::transport::transport_spec::port server_port{
                headers::transport::transport_spec::port::port_type::server,
                boost::get<headers::transport::transport_spec::port_number>(client_port.port_numbers) + 2};
        choosen_spec.parameters.push_back(server_port);
    } else {
        choosen_spec.parameters.push_back(*parameter);
    }


    return choosen = choosen_spec;
}

std::unique_ptr<std::istream> get_jpeg_stream(const rtsp::request_uri &uri, const rtsp::fileapi::path &ressource_root) {
    std::unique_ptr<std::istream> stream{};

    std::string file_name;
    using namespace boost::spirit;
    bool parsed = qi::parse(uri.cbegin(), uri.cend(),
                            (qi::lit("rtsp:") | qi::lit("rtspu:")) >> qi::lit("//") >> qi::omit[
                                    *(rtsp::ns::char_ - "/")
                            ] >> qi::lit("/") >> rtsp::ns::alnum >> *rtsp::ns::print, file_name);

    if (!parsed)
        return stream;
    try {
        auto ressource_path = rtsp::fileapi::canonical(file_name, ressource_root);
        stream = std::make_unique<std::fstream>(ressource_path.string());
    } catch (const boost::filesystem::filesystem_error &e) {
        return stream;
    }
    return stream;
}

std::pair<rtsp::response, rtsp::body> rtsp::methods::setup(rtsp::rtsp_server_session &session,
                                                           const rtsp::internal_request &request,
                                                           const fileapi::path &ressource_root,
                                                           boost::asio::io_context &io_context) {
    rtsp::response response{common_response_sekeleton(session, request)};

    boost::optional<headers::mkn_option_parameters> options{};
    if (request.second.count("require")) {
        std::vector<string> option_tags;
        namespace qi = boost::spirit::qi;
        rtsp::headers::common_rules<std::string::const_iterator> rules{};
        auto begin = request.second.at("require").cbegin();
        auto end = request.second.at("require").cend();
        qi::phrase_parse(begin, end, rules.require_, boost::spirit::ascii::space, option_tags);
        if (begin != end) {
            response.status_code = 400;
            response.reason_phrase = rtsp::string{"Bad Request: Could not read transport header after"} +
                                     rtsp::string{begin, end};
            return std::make_pair<rtsp::response, rtsp::body>(std::move(response), {});
        }

        std::vector<string> unsupported_option_tags;
        std::copy_if(option_tags.begin(), option_tags.end(), std::back_inserter(unsupported_option_tags),
                     [](const auto &option_tag) -> bool {
                         return option_tag != headers::mkn_option_tag;
                     });

        if (!unsupported_option_tags.empty()) {
            response.status_code = 551;
            response.reason_phrase = "Option not supported";
            for (const auto &option_tag : unsupported_option_tags) {
                response.headers.emplace_back("Unsupported", option_tag);
            }
            return std::make_pair<rtsp::response, rtsp::body>(std::move(response), {});
        }
        if (request.second.count(headers::mkn_option_header) == 0) {
            response.status_code = 400;
            response.reason_phrase = headers::mkn_option_header + " header missing";
            return std::make_pair<rtsp::response, rtsp::body>(std::move(response), {});
        }

        begin = request.second.at(headers::mkn_option_header).cbegin();
        end = request.second.at(headers::mkn_option_header).cend();
        headers::mkn_option_parameters opt{};
        rtsp::headers::mkn_bernoulli_channel_parameter_grammar<std::string::const_iterator> grammar{};
        qi::parse(begin, end, grammar, opt);
        if (begin != end) {
            response.status_code = 400;
            response.reason_phrase = rtsp::string{"Bad Request: Could not read "}
                                     + headers::mkn_option_header + " header after" +
                                     rtsp::string{begin, end};
            return std::make_pair<rtsp::response, rtsp::body>(std::move(response), {});
        }
        options = opt;
    }

    rtsp::headers::transport request_transport{};

    if (request.second.count("transport")) {
        rtsp::headers::transport_grammar<std::string::const_iterator> transport_grammar{};
        auto begin = request.second.at("transport").cbegin();
        auto end = request.second.at("transport").cend();

        boost::spirit::qi::phrase_parse(begin, end, transport_grammar, boost::spirit::ascii::space, request_transport);

        if (begin != end) {
            response.status_code = 400;
            response.reason_phrase = rtsp::string{"Bad Request: Could not read transport header after"} +
                                     rtsp::string{begin, end};
            return std::make_pair<rtsp::response, rtsp::body>(std::move(response), {});
        }
    }

    auto choosen_transport = get_acceptable_transport(request_transport);
    if (!choosen_transport) {
        response.status_code = 461;
        response.reason_phrase = rtsp::string{"Unsupported Transport: The Transport field did not contain a "
                                              "supported transport specification."};
        return std::make_pair<rtsp::response, rtsp::body>(std::move(response), {});
    }

    if (session.session_state() != rtsp_server_session::state::init) {
        response.status_code = 455;
        response.reason_phrase = rtsp::string{"Method Not Valid In This State"};
        return std::make_pair<rtsp::response, rtsp::body>(std::move(response), {});
    }

    auto ressource_path = get_jpeg_stream(request.first.uri, ressource_root);
    if (!ressource_path) {
        response.status_code = 404;
        response.reason_phrase = rtsp::string{"Ressource \""} + request.first.uri + "\" Not Found";
        return std::make_pair<rtsp::response, rtsp::body>(std::move(response), {});
    }

    const auto &client_port = boost::get<headers::transport::transport_spec::port_number>(
            boost::get<headers::transport::transport_spec::port>(choosen_transport.value().parameters.at(1)).
                    port_numbers);
    const auto &server_port = boost::get<headers::transport::transport_spec::port_number>(
            boost::get<headers::transport::transport_spec::port>(choosen_transport.value().parameters.at(3)).
                    port_numbers);
    const auto &ssrc = boost::get<headers::transport::transport_spec::ssrc>(choosen_transport.value().parameters.at(2));

    session.rtp_session = std::make_unique<rtp::unicast_jpeg_rtp_sender>(
            boost::asio::ip::udp::endpoint{session.last_seen_request_address, client_port},
            server_port,
            ssrc,
            std::move(ressource_path),
            io_context
    );

    session.set_session_state(rtsp_server_session::state::ready);
    rtsp::headers::transport response_transport{};
    response_transport.specifications.push_back(choosen_transport.value());

    rtsp::string transport_string;
    rtsp::headers::generate_transport_header_grammar<std::back_insert_iterator<std::string>> transport_gen_grammar{};
    boost::spirit::karma::generate(std::back_inserter(transport_string), transport_gen_grammar, response_transport);
    response.headers.emplace_back("Transport", std::move(transport_string));

    return std::make_pair<rtsp::response, rtsp::body>(std::move(response), {});
}


std::pair<rtsp::response, rtsp::body> rtsp::methods::teardown(rtsp::rtsp_server_session &session,
                                                              const rtsp::internal_request &request) {
    rtsp::response response{common_response_sekeleton(session, request)};


    return std::make_pair<rtsp::response, rtsp::body>(std::move(response), {});
}

std::pair<rtsp::response, rtsp::body>
rtsp::methods::play(rtsp_server_session &session, const internal_request &request) {
    rtsp::response response{common_response_sekeleton(session, request)};

    if (session.session_state() == rtsp_server_session::state::playing)
        return std::make_pair<rtsp::response, rtsp::body>(std::move(response), {});
    else if (session.session_state() != rtsp_server_session::state::ready) {
        response.status_code = 455;
        response.reason_phrase = "Method Not Valid in This State";
        return std::make_pair<rtsp::response, rtsp::body>(std::move(response), {});
    }

    session.rtp_session->start();
    session.set_session_state(rtsp_server_session::state::playing);

    return std::make_pair<rtsp::response, rtsp::body>(std::move(response), {});
}

std::pair<rtsp::response, rtsp::body>
rtsp::methods::pause(rtsp_server_session &session, const internal_request &request) {
    rtsp::response response{common_response_sekeleton(session, request)};

    if (session.session_state() != rtsp_server_session::state::playing) {
        response.status_code = 455;
        response.reason_phrase = "Method Not Valid in This State";
        return std::make_pair<rtsp::response, rtsp::body>(std::move(response), {});
    }

    session.rtp_session->stop();
    session.set_session_state(rtsp_server_session::state::ready);

    return std::make_pair<rtsp::response, rtsp::body>(std::move(response), {});
}

