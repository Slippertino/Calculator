#pragma once

#include <QString>
#include <QMap>
#include <QTranslator>
#include <QCoreApplication>
#include "model/status.hpp"
#include "model/op.hpp"

class Translator : public QObject {

	Q_OBJECT

public:
	static QString get(calculator::status_type status, calculator::op_ptr op = nullptr) {
        auto res = tr(status_mappings_.value(status));
		auto isNumberError = 
			status == calculator::status_type::INVALID_ARGUMENT || 
			status == calculator::status_type::NUMBER_OVERFLOW;

		if (isNumberError && op) {
			QString op_msg;

			auto opt = op->type();
			if (ops_invalid_hints_.contains(opt))
				op_msg = tr(ops_invalid_hints_.value(opt));
			else
				op_msg = "...";

			res = QString("%1 %2").arg(res).arg(op_msg);
		}

		return res;
	}

private:
	static inline const QMap<calculator::status_type, const char*> status_mappings_ = {
        { calculator::status_type::OK,					QT_TR_NOOP("ОК")									},
		{ calculator::status_type::UNKNOWN_ERROR,		QT_TR_NOOP("Неизвестная ошибка")					},
		{ calculator::status_type::TOO_LONG_NUMBER,		QT_TR_NOOP("Слишком большое число")					},
		{ calculator::status_type::INVALID_NUMBER,		QT_TR_NOOP("Некорректное число")					},
		{ calculator::status_type::UNKNOWN_SYMBOL,		QT_TR_NOOP("Неизвестный символ")					},
		{ calculator::status_type::INVALID_EXPR,		QT_TR_NOOP("Некорректное выражение")				},
		{ calculator::status_type::INVALID_EVAL,		QT_TR_NOOP("Некорректное выражение")				},
        { calculator::status_type::NUMBER_OVERFLOW,		QT_TR_NOOP("Переполнение при")						},
        { calculator::status_type::INVALID_ARGUMENT,	QT_TR_NOOP("Некорректный аргумент при")				},
	};

	static inline const QMap<calculator::symbol_type, const char*> ops_invalid_hints_ = {
        { calculator::symbol_type::ADD,		QT_TR_NOOP("сложении")					},
        { calculator::symbol_type::MULT,	QT_TR_NOOP("умножении")					},
        { calculator::symbol_type::POW,		QT_TR_NOOP("возведении в степень")		},
        { calculator::symbol_type::MINUS,	QT_TR_NOOP("минусе")					},
        { calculator::symbol_type::COS,		QT_TR_NOOP("косинусе")					},
        { calculator::symbol_type::SIN,		QT_TR_NOOP("синусе")					},
		{ calculator::symbol_type::FACT,	QT_TR_NOOP("факториале")				},
		{ calculator::symbol_type::SQRT,	QT_TR_NOOP("квадратном корне")			},
		{ calculator::symbol_type::MOD,		QT_TR_NOOP("взятии остатка")			},
		{ calculator::symbol_type::TAN,		QT_TR_NOOP("тангенсе")					},
		{ calculator::symbol_type::ACOS,	QT_TR_NOOP("арккосинусе")				},
		{ calculator::symbol_type::ASIN,	QT_TR_NOOP("арксинусе")					},
		{ calculator::symbol_type::ATAN,	QT_TR_NOOP("арктангенсе")				},
		{ calculator::symbol_type::LN,		QT_TR_NOOP("натуральном логарифме")		},
		{ calculator::symbol_type::LG,		QT_TR_NOOP("десятичном логарифме")		},
		{ calculator::symbol_type::DIV,		QT_TR_NOOP("делении")					},
	};
};
