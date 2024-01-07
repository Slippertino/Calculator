#pragma once

#include <optional>
#include <QFontMetrics>
#include "expression.hpp"
#include "formatters.hpp"
#include "translator.hpp"

class Presenter {
public:
	QString getText()	const;
	int		getCursor() const;
	QString getStatus() const;

    void setPosition(int pos);
	void setStatusMetrics(QFontMetrics&& fm);
	void setStatusWidth(int maxSize);

    void onInsert(const QString& s);
    void onRemove(int count);

    void onFraction();
    void onBracket();
    void onInvert();
    void onClear();
    QString onEval();

private:
	QString buildExpressionHint(QString body, QString res) const;

private:
	using StableFormatter = Formatter<Expression,
		OperationComplementer,
		BinaryOperationSpaceComplementer,
		UnaryOperationLeftBracketComplementer,
        MinusComplementer,
		NumberGapFormatter
	>;

	using EvalFormatter = Formatter<Expression,
		Reformatter,
		OperationComplementer,
		BinaryOperationSpaceComplementer,
		UnaryOperationLeftBracketComplementer,
		MinusComplementer,
		NumberGapFormatter,
		RightBracketComplementer
	>;

private:
	QFontMetrics statusMetrics_{ QFont{} };
	int statusWidth_;

	Expression expr_;
};
