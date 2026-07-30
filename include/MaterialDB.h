#pragma once

#include <QString>
#include <QList>

struct MaterialData {
	QString name;			// 재질명
	double specificGravity;	// 비중 (단위: g/cm^3)
	int costPerKg;			// kg당 단가
};

inline QList<MaterialData> getMaterialDatabase() {
	QList<MaterialData> db;

	db.append({ "AL6061 (알루미늄)",2.7,5000 });
	db.append({ "S45C (탄소강)", 7.85, 2500 });
	db.append({ "SUS304 (스테인리스)", 7.93, 7000 });
	db.append({ "POM (아세탈수지)", 1.41, 6000 });

	return db;
}