#include "presenter.hpp"

QString Presenter::getText() const {
    return expr_.getExpression();
}

int Presenter::getCursor() const {
    return expr_.getPosition();
}

QString Presenter::getStatus() const {
    auto [res, st, op] = expr_.eval();
    if (st == calculator::status_type::INVALID_EVAL)
        return QString{};
    if (st != calculator::status_type::OK)
        return Translator::get(st, op);
    auto expr = Expression{ expr_ };
    expr.update<EvalFormatter>();
    res.update<StableFormatter>();
    return buildExpressionHint(expr.getExpression(), res.getExpression());
}

void Presenter::setPosition(int pos) {
    expr_.setPosition(pos);
}

void Presenter::setStatusMetrics(QFontMetrics&& fm) {
    statusMetrics_ = std::move(fm);
}

void Presenter::setStatusWidth(int maxSize) {
    statusWidth_ = maxSize;
}

void Presenter::onInsert(const QString& s) {
    expr_.insert(s);
    expr_.update<StableFormatter>();
}

void Presenter::onRemove(int count) {
    if (count >= 0)
        expr_.remove(count);
    else {
        count *= -1;
        expr_.removeRange({{ expr_.getPosition() - count, count }});
    }
    expr_.update<StableFormatter>();
}

void Presenter::onFraction() {
    expr_
        .pushFront("1/(")
        .pushBack(")");
    expr_.update<StableFormatter>();
}

void Presenter::onBracket() {
    auto open_cnt = expr_.getOpenBracketsCount();
    QChar ch = (open_cnt > 0) ? ')' : '(';
    onInsert(QString{ 1, ch });
}

void Presenter::onInvert() {
    expr_
        .pushFront("-(")
        .pushBack(")");
    expr_.update<StableFormatter>();
}

void Presenter::onClear() {
    expr_.clear();
    expr_.update();
}

QString Presenter::onEval() {
    auto [res, st, op] = expr_.eval();
    auto status = (st != calculator::status_type::OK) 
        ? Translator::get(st, op) 
        : getStatus();
    expr_.update<EvalFormatter>();
    expr_.evalAndUpdate();
    expr_.update<StableFormatter>();
    return status;
}

QString Presenter::buildExpressionHint(QString body, QString res) const {
    static const QString hintMask{ "%1 = %2" }, rest{ "..." };
    if (body == res)
        return body;

    auto out = QString(hintMask).arg(body).arg(res);
    auto sz = statusMetrics_.width(out);
    if (sz <= statusWidth_)
        return out;

    auto res_sz = res.size();
    int l{ res_sz }, r{ body.size() + res_sz };
    while (r - l > 1) {
        auto mid = (r + l) / 2;
        out = QString(hintMask).arg(body.mid(body.size() - mid, mid).append(rest)).arg(res);
        if (statusMetrics_.width(out) > statusWidth_)
            r = mid;
        else
            l = mid;
    }

    return QString(hintMask).arg(body.mid(body.size() - l, l).append(rest)).arg(res);;
}