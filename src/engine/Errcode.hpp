#include <string>
#include <bbt/errcode/Errcode.hpp>
#include <optional>

namespace bbt::http
{

class Errcode;

enum emErr
{
    Failed = 0,
    BadParams = 1,
};

typedef std::optional<Errcode> ErrOpt;

class Errcode:
    public bbt::errcode::Errcode<emErr>
{
public:
    typedef bbt::errcode::Errcode<emErr> TBase;

    explicit Errcode(const std::string& msg, emErr err_type = emErr::Failed):TBase(msg, err_type) {}
    virtual ~Errcode() {}

    const std::string& What() { GetMsg(); }
};

} // namespace bbt::http
