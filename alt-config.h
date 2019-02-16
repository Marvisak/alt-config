#pragma once

#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <deque>
#include <iostream>
#include <algorithm>
#include <cctype>

namespace alt::config
{
	namespace detail
	{
		inline std::string Unescape(const std::string& str)
		{
			std::string res;

			auto it = str.begin();

			while (it != str.end())
			{
				char c = *it++;

				if (c == '\\' && it != str.end())
				{
					c = *it++;

					switch (c)
					{
					case 'n':
					case '\n':
						res += '\n';
						break;
					case 'r':
						res += '\r';
						break;
					default:
						(res += '\\') += c;
					}

					continue;
				}
				
				res += c;
			}

			// trim trailing spaces
			res.erase(std::find_if(res.rbegin(), res.rend(), [](int ch) {
				return !std::isspace(ch);
			}).base(), res.end());

			return res;
		}
	}

	class Error : public std::exception
	{
		std::string err;

	public:
		Error(const std::string& _err) : err{ _err } { }
		const char* what() const noexcept override { return err.c_str(); }
	};

	class Node
	{
	public:
		using Scalar = std::string;
		using List = std::vector<Node>;
		using Dict = std::map<std::string, Node>;

		enum class Type
		{
			NONE,
			SCALAR,
			LIST,
			DICT,
		};

		Node() :
			type(Type::NONE),
			val(new Value)
		{

		}

		Node(bool _val) :
			type(Type::SCALAR),
			val(new ValueScalar{ _val ? "true" : "false" })
		{

		}

		Node(double _val) :
			type(Type::SCALAR)
		{
			std::ostringstream ss;
			ss.precision(15);
			ss << _val;

			val = new ValueScalar{ ss.str() };
		}

		Node(int val) : Node(static_cast<double>(val)) { }
		Node(unsigned val) : Node(static_cast<double>(val)) { }
		Node(int64_t val) : Node(static_cast<double>(val)) { }
		Node(uint64_t val) : Node(static_cast<double>(val)) { }

		Node(const Scalar& _val) :
			type(Type::SCALAR),
			val(new ValueScalar{ _val })
		{

		}

		Node(const char* val) : Node(std::string{ val }) { }

		Node(const List& _val) :
			type(Type::LIST),
			val(new ValueList{ _val })
		{

		}

		Node(const Dict& _val) :
			type(Type::DICT),
			val(new ValueDict{ _val })
		{

		}

		Node(const Node& that) :
			type(that.type),
			val(that.val->Copy())
		{

		}

		~Node() { delete val; }

		Node& operator=(const Node& that)
		{
			type = that.type;
			delete val;
			val = that.val->Copy();

			return *this;
		}

		bool IsNone() const { return type == Type::NONE; }
		bool IsScalar() const { return type == Type::SCALAR; }
		bool IsList() const { return type == Type::LIST; }
		bool IsDict() const { return type == Type::DICT; }

		bool ToBool() { return val->ToBool(); }
		bool ToBool(bool def) { return val->ToBool(def); }

		double ToNumber() { return val->ToNumber(); }
		double ToNumber(double def) { return val->ToNumber(def); }

		std::string ToString() { return val->ToString(); }
		std::string ToString(const std::string& def) { return val->ToString(def); }

		List& ToList() { return val->ToList(); }
		Dict& ToDict() { return val->ToDict(); }

		Node& operator[](std::size_t idx) { return val->Get(idx); }
		Node& operator[](const std::string& key) { return val->Get(key); }
		Node& operator[](const char* key) { return val->Get(key); }

		operator bool() { return !IsNone(); }

		friend std::ostream& operator<<(std::ostream& os, const Node& node)
		{
			node.val->Print(os);
			return os;
		}

	protected:
		Node(Type _type) : type(_type) { };

	private:
		class Value
		{
		public:
			virtual ~Value() = default;

			virtual Value* Copy() { return new Value; }

			virtual bool ToBool() { throw Error{ "Invalid cast" }; }
			virtual bool ToBool(bool def) { return def; }

			virtual double ToNumber() { throw Error{ "Invalid cast" }; }
			virtual double ToNumber(double def) { return def; }

			virtual std::string ToString() { throw Error{ "Invalid cast" }; }
			virtual std::string ToString(const std::string& def) { return def; }

			virtual List& ToList() { throw Error{ "Invalid cast" }; }
			virtual Dict& ToDict() { throw Error{ "Invalid cast" }; }

			virtual Node& Get(std::size_t idx) { throw Error{ "Not a list" }; }
			virtual Node& Get(const std::string& key) { throw Error{ "Not a dict" }; }

			virtual void Print(std::ostream& os, int indent = 0) { os << "Node{}"; }
		};

		Type type;
		Value* val;

		class ValueScalar : public Value
		{
		public:
			ValueScalar(const Scalar& _val) : val(_val) { }

			Value* Copy() { return new ValueScalar{ val }; }

			bool ToBool() override
			{
				if (val == "true" || val == "yes")
					return true;
				else if (val == "false" || val == "no")
					return false;

				throw Error{ "Not a bool" };
			}

			double ToNumber() override
			{
				try
				{
					return std::stod(val);
				}
				catch (const std::invalid_argument& e)
				{
					throw Error{ "Not a number" };
				}
			}

			std::string ToString() override
			{
				return val;
			}

			bool ToBool(bool def) override { return ToBool(); }
			double ToNumber(double def) override { return ToNumber(); }
			std::string ToString(const std::string& def) override { return ToString(); }

		private:
			void Print(std::ostream& os, int indent = 0) override { os << val; }

		private:
			Scalar val;
		};

		class ValueList : public Value
		{
		public:
			ValueList(const List& _val) : val(_val) { }

			Value* Copy() { return new ValueList{ val }; }

			List& ToList() override
			{
				return val;
			}

			Node& Get(std::size_t idx) override
			{
				static Node none;

				if (idx >= val.size())
					return none;

				return val.at(idx);
			}

		private:
			List val;
		};

		class ValueDict : public Value
		{
		public:
			ValueDict(const Dict& _val) : val(_val) { }

			Value* Copy() { return new ValueDict{ val }; }

			Dict& ToDict() override
			{
				return val;
			}

			Node& Get(const std::string& key) override
			{
				return val[key];
			}

		private:
			Dict val;
		};
	};

	class Parser
	{
	public:
		Parser(std::istream& is) :
			buffer{ (std::istreambuf_iterator<char>(is)),
				std::istreambuf_iterator<char>() }
		{

		}

		Parser(const std::vector<char>& _buffer) :
			buffer(_buffer)
		{

		}

		Parser(const char* data, size_t size) :
			buffer(data, data + size)
		{

		}

		Node Parse()
		{
			Tokenize();
			auto begin = tokens.begin();
			return Parse(begin);
		}

	private:
		struct Token
		{
			enum Type
			{
				ARRAY_START,
				ARRAY_END,

				DICT_START,
				DICT_END,

				KEY,
				SCALAR
			} type;

			std::string value;
		};

		std::size_t Unread() { return buffer.size() - readPos; }
		char Peek(std::size_t offset = 0) { return *(buffer.data() + readPos + offset); }
		char Get() { return *(buffer.data() + readPos++); }
		void Skip(std::size_t n = 1) { readPos += n; }

		void SkipToNextToken()
		{
			while (Unread() > 0)
			{
				if (Peek() == ' ' || Peek() == '\n' || Peek() == ',')
					Skip();
				else if (Peek() == '#')
				{
					Skip();

					while (Unread() > 0 && Peek() != '\n' && Peek() != '#')
						Skip();

					if (Unread() > 0) Skip();
				}
				else
					break;
			}
		}

		void Tokenize()
		{
			tokens.push_back({ Token::DICT_START, "" });

			while (Unread() > 0)
			{
				SkipToNextToken();

				if (Unread() == 0)
					break;

				if (Peek() == '[')
				{
					Skip();
					tokens.push_back({ Token::ARRAY_START, "" });
				}
				else if (Peek() == ']')
				{
					Skip();
					tokens.push_back({ Token::ARRAY_END, "" });
				}
				else if (Peek() == '{')
				{
					Skip();
					tokens.push_back({ Token::DICT_START, "" });
				}
				else if (Peek() == '}')
				{
					Skip();
					tokens.push_back({ Token::DICT_END, "" });
				}
				else
				{
					std::string val;

					if (Peek() == '\'' || Peek() == '"')
					{
						char start = Get();

						while (Unread() > 0 && Peek() != start)
						{
							if (Peek() == '\n' || Peek() == '\r')
							{
								if (Get() == '\r' && Peek() == '\n')
									Skip();

								val += '\n';
								continue;
							}

							val += Get();
						}

						if (Unread() == 0)
							throw Error("Unexpected end of file");

						Skip();
					}
					else
					{
						while (Unread() > 0 &&
							Peek() != '\n' &&
							Peek() != ':' &&
							Peek() != ',' &&
							Peek() != ']' &&
							Peek() != '}' &&
							Peek() != '#')
						{
							val += Get();
						} 
					}

					val = detail::Unescape(val);

					if (Unread() > 0 && Peek() == ':')
						tokens.push_back({ Token::KEY, val });
					else
						tokens.push_back({ Token::SCALAR, val });

					if (Unread() > 0 && (Peek() == ':' || Peek() == ','))
						Skip();
				}
			}

			tokens.push_back({ Token::DICT_END, "" });
		}

		Node Parse(std::deque<Token>::iterator& tok)
		{
			switch (tok->type)
			{
			case Token::SCALAR:
				return { (tok++)->value };
			case Token::ARRAY_START:
			{
				Node::List list;

				++tok;
				while (tok != tokens.end() && tok->type != Token::ARRAY_END)
					list.push_back(Parse(tok));

				++tok;
				return { list };
			}
			case Token::DICT_START:
			{
				Node::Dict dict;

				++tok;
				while (tok != tokens.end() && tok->type != Token::DICT_END)
				{
					if (tok->type != Token::KEY)
						throw Error("key expected");

					std::string key = tok->value;

					dict.insert({ key, Parse(++tok) });
				}

				++tok;
				return { dict };
			}
			}

			throw Error("invalid token");
		}

		std::vector<char> buffer;
		std::size_t readPos = 0;
		std::deque<Token> tokens;
	};

	class Emitter
	{
	public:
		void Emit(Node& node, std::ostream& os, int indent = 0)
		{
			std::string _indent(indent * 2, ' ');

			if (node.IsScalar())
			{
				os << '\'' << node.ToString() << "'\n";
			}
			else if (node.IsList())
			{
				os << "[\n";

				for (auto& _node : node.ToList())
				{
					os << _indent;
					Emit(_node, os, indent + 1);
				}

				os << _indent << "]\n";
			}
			else if (node.IsDict())
			{
				if (indent > 0)
					os << "{\n";

				for (auto& pair : node.ToDict())
				{
					if (pair.second.IsNone())
						continue;

					os << _indent << pair.first << ": ";
					Emit(pair.second, os, indent + 1);
				}

				if (indent > 0)
					os << _indent << "}\n";
			}
		}
	};
};
