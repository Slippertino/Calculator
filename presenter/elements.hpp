#pragma once

#include <memory>
#include <list>
#include <bitset>
#include <map>
#include <QString>
#include "trie.hpp"
#include "model/lexer.hpp"
#include "settings.hpp"
#include "optimizedlist.hpp"

class DelimetersContainer {
public:
    int getDelimetersSizeByPos(int pos) const;

protected:
    struct Delimeter {
        int pos;
        int size;

        bool isInside(int p) const noexcept {
            return pos <= p && p < pos + size;
        }
    };

protected:
    DelimetersContainer() = delete;
    DelimetersContainer(QString& value, const std::list<Delimeter>& delims);

    void insertDelimeter(int pos, const QString& val);
    std::list<Delimeter>::iterator removeDelimeter(std::list<Delimeter>::iterator it, int offset);
    void clearFromDelimeters();
    std::list<Delimeter>::iterator find(int pos);
    void shiftDelimeters(std::list<Delimeter>::iterator begin, int offset);
    QString removeDelimeters() const;

protected:
    QString &value_;
    std::list<Delimeter> delimeters_;
};

class Block;
using BlockPtr = std::unique_ptr<Block>;
using BlockPtrList = OptimizedListWrapper<BlockPtr, 8>;

class Block : public DelimetersContainer {
public:
    static constexpr int kMaxFormattersCount = 10;
    
    using format_t = std::bitset<kMaxFormattersCount>;
    static inline const format_t kFullFlags = format_t{ std::string(kMaxFormattersCount, '1') };

public:
    Block() = delete;
    Block(
        int start, 
        calculator::token_type type, 
        const QString &value, 
        bool isMutable = true,
        format_t formatFlags = kFullFlags,
        const std::list<Delimeter> &delimeters = {}
    );
    Block(const Block& bl);

    static BlockPtrList create(int start, const QString& str);
    static BlockPtrList merge(int start,  BlockPtrList& blocks);

    int begin() const noexcept;
    int end() const noexcept;
    int size() const noexcept;
    bool empty() const noexcept;
    bool isMutable() const noexcept;
    calculator::token_type type() const noexcept;
    bool isInside(int pos) const noexcept;
    bool canSplit(int pos) const noexcept;

    format_t& formatFlags() noexcept;
    void shift(int size) noexcept;

    int insert(int pos, const Block& s);
    int remove(int pos, int count);
    bool merge(const Block& s);
    std::tuple<BlockPtrList, BlockPtrList, int> split(int pos);
    int insertDelimeter(int pos, const QString &delim);
    int clearFromDelimeters();

    virtual QString toString(bool delims = true) const;
    virtual BlockPtr clone() const;
    
    virtual ~Block() = default;

protected:
    void update(const QString& s);

    virtual int insertMutableImpl(int pos, const Block& s);
    virtual int removeMutableImpl(int pos, int count);

protected:
    bool mutable_;
    calculator::token_type type_;
    int start_;
    int size_;
    format_t format_flags_;
    QString value_;
};

class Symbol final : public Block {
public:
    Symbol() = delete;
    Symbol(
        int start, 
        const QString& value, 
        calculator::symbol_type symbol_type, 
        format_t formatFlags = kFullFlags,
        const std::list<Delimeter>& delimeters = {}
    );

    calculator::symbol_type symbol_type() const noexcept;
    int refresh();

    BlockPtr clone() const override final;

protected:
    int insertMutableImpl(int pos, const Block& s) override final;
    int removeMutableImpl(int pos, int count) override final;

private:
    void updateType(const QString& val);
    QString transformToTree(const QString& val, int threshold);

private:
    static inline const Trie<wchar_t> symbols_trie_ = [](){ 
        auto symbols = calculator::lexer::get_symbols();
        return Trie<wchar_t>(symbols.begin(), symbols.end());
    }();

    calculator::symbol_type symbol_type_;
};

class Number final : public Block {
public:
    Number() = delete;
    Number(
        int start, 
        const QString &value, 
        format_t formatFlags = kFullFlags, 
        const std::list<Delimeter>& delimeters = {}
    );
    Number(
        int start, 
        const calculator::number_t& value,
        format_t formatFlags = kFullFlags,
        const std::list<Delimeter>& delimeters = {}
    );

    calculator::number_t get() const;
    BlockPtr clone() const override final;

private:
    int insertMutableImpl(int pos, const Block& s) override final;
    QString convertNumberToString(const calculator::number_t& num) const;

private:
    calculator::number_t calculated_number_;
};

class Space : public Block {
public:
    Space(int start = 0);
};

class LeftBracket : public Block {
public:
    LeftBracket(int start = 0);
};

class RightBracket : public Block {
public:
    RightBracket(int start = 0);
};