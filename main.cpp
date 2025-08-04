#include <algorithm>
#include <iostream>
#include <vector>
#include <memory>
#include <cassert>

struct context;

struct data
{
    using addr = size_t;

    int i;
    std::vector<addr> v;
    void (*op)(context &);
};

enum data_type
{
    INVALID,
    INT,
    CHAR,
    VECTOR,
    OP,

    COUNT_,
    FIRST_ = INVALID,
    LAST_ = COUNT_ - 1,
};

struct value
{
    data_type t = INVALID;
    data d;
    static value value_int(const int x) { value v {INT}; v.d.i = x; return v; }
    static value value_char(const char x) { value v {CHAR}; v.d.i = x; return v; }
    static value value_vector(std::vector<data::addr> a) { value v {VECTOR}; v.d.v = std::move(a); return v; }
    static value value_op(void(*op)(context &)) { value v {OP}; v.d.op = op; return v; }
};

struct context
{
    virtual ~context() = default;
    virtual value input_stack_pop(bool exec_op = true) = 0;
    virtual const value &input_take_by_addr(data::addr) const = 0;
    virtual void output_stack_push(value) = 0;
    virtual void output_write_by_addr(data::addr, value) = 0;
    virtual data::addr next_unused_addr() = 0;
    virtual std::ostream &os() { return std::cout; }
    virtual std::istream &is() { return std::cin; }
};

struct context_impl : context
{
    std::vector<value> stack;
    std::vector<value> pile;
    size_t pile_size = 0;
    value invalid_;
    value input_stack_pop(bool exec_op = true) override;
    const value & input_take_by_addr(data::addr a) const override;
    void output_stack_push(value v) override;
    void output_write_by_addr(data::addr a, value v) override;
    data::addr next_unused_addr() override;
};

void ident(context &ctx);
void print(context &ctx);
void increment(context &ctx);
void repeat_op_x_times(context &ctx);
void summ_two_ints(context &ctx);
void generate_index_array(context &ctx);
void break_array(context &ctx);
void get_user_int(context &ctx);
void get_user_string(context &ctx);

int main(int, char **)
{
    context_impl c;

    std::cout << "\nSumm fold:\n";
    c.output_stack_push(value::value_int(10));
    c.output_stack_push(value::value_op(&generate_index_array));
    c.output_stack_push(value::value_op(&print));
    c.output_stack_push(value::value_op(&break_array));
    c.output_stack_push(value::value_op(&print));
    c.output_stack_push(value::value_op(&summ_two_ints));
    c.output_stack_push(value::value_int(9));
    c.output_stack_push(value::value_op(&repeat_op_x_times));
    c.output_stack_push(value::value_op(&print));
    c.input_stack_pop();

    std::cout << "\nBranch summ:\n";
    c.output_stack_push(value::value_int(21000));
    c.output_stack_push(value::value_int(21000));
    c.output_stack_push(value::value_int(42));
    c.output_stack_push(value::value_int(69));
    c.output_stack_push(value::value_op(&summ_two_ints));
    c.output_stack_push(value::value_op(&summ_two_ints));
    c.output_stack_push(value::value_op(&summ_two_ints));
    c.output_stack_push(value::value_op(&print));
    c.input_stack_pop();

    // get_user_int(c);
    // print(c);
    // get_user_string(c);
    // print(c);
    return 0;
}





// impl

void ident(context &ctx)
{
    ctx.output_stack_push(ctx.input_stack_pop());
}

void print_(context &ctx, const value &v)
{
    switch (v.t) {
    case INT:
        ctx.os() << v.d.i;
        break;
    case CHAR:
        ctx.os() << '\'' << v.d.i << '\'';
        break;
    case OP:
        ctx.os() << "OP(" << v.d.op << ')';
        break;
    case VECTOR: {
        const bool is_str = std::all_of(v.d.v.begin(), v.d.v.end(), [&ctx](const int i) {
            return ctx.input_take_by_addr(i).t == CHAR;
        });
        if (is_str) {
            ctx.os() << '"';
            for (const int i : v.d.v)
                ctx.os() << static_cast<char>(ctx.input_take_by_addr(i).d.i);
            ctx.os() << '"';
            break;
        }
        ctx.os() << '[';
        for (int i = 0; i < v.d.v.size(); i++) {
            if (i) ctx.os() << ' ';
            print_(ctx, ctx.input_take_by_addr(v.d.v[i]));
        }
        ctx.os() << ']';
        break;
    }
    default:
        static_assert(COUNT_ == 5);
        assert(false && "unreachable");
        break;
    }
}

void print(context &ctx)
{
    value v = ctx.input_stack_pop();
    print_(ctx, v);
    ctx.os() << '\n';
    ctx.output_stack_push(std::move(v));
}
void assert_numeric_value(const value &v) // TODO: remove it!
{
    if (v.t != INT && v.t != CHAR && v.t != OP) // since it's not statically typed, op output is OK
        throw std::runtime_error("value expected to have numeric type!");
}

void increment(context &ctx)
{
    ctx.output_stack_push(value::value_int(ctx.input_stack_pop().d.i + 1));
}

void repeat_op_x_times(context &ctx)
{
    const value x = ctx.input_stack_pop();
    const value op = ctx.input_stack_pop(false);
    for (int i = 0; i < x.d.i; ++i)
       op.d.op(ctx);
}

void summ_two_ints(context &ctx)
{
    ctx.output_stack_push(value::value_int(
        ctx.input_stack_pop().d.i + ctx.input_stack_pop().d.i));
}

void generate_index_array(context &ctx)
{
    const int size = ctx.input_stack_pop().d.i;
    std::vector<data::addr> as(size);
    for (int i = 0; i < size; i++) {
        as[i] = ctx.next_unused_addr();
        ctx.output_write_by_addr(as[i], value::value_int(i));
    }
    ctx.output_stack_push(value::value_vector(std::move(as)));
}

void break_array(context &ctx)
{
    const value v = ctx.input_stack_pop();
    const std::vector<data::addr> &as = v.d.v;
    for (auto i = as.rbegin(); i != as.rend(); ++i)
        ctx.output_stack_push(ctx.input_take_by_addr(*i));
}

void get_user_int(context &ctx)
{
    int x;
    ctx.is() >> x;
    ctx.output_stack_push(value::value_int(x));
}

void get_user_string(context &ctx)
{
    std::string s;
    ctx.is() >> s;

    std::vector<data::addr> is;
    is.reserve(s.size());
    for (const char c : s) {
        const auto addr = ctx.next_unused_addr();
        ctx.output_write_by_addr(addr, value::value_char(c));
        is.push_back(addr);
    }
    ctx.output_stack_push(value::value_vector(std::move(is)));
}

value context_impl::input_stack_pop(const bool exec_op)
{
    if (stack.empty())
        throw std::runtime_error("stack is empty, but expecting a value!");
    auto v = stack.back();
    stack.pop_back();
    if (v.t != OP || !exec_op)
        return v;
    v.d.op(*this);
    return input_stack_pop();
}

const value & context_impl::input_take_by_addr(const data::addr a) const
{
    return pile.at(a);
}

void context_impl::output_stack_push(value v)
{
    stack.emplace_back(std::move(v));
}

void context_impl::output_write_by_addr(const data::addr a, value v)
{
    pile[a] = std::move(v);
}

data::addr context_impl::next_unused_addr()
{
    const data::addr a = pile.size();
    pile.emplace_back();
    return a;
}