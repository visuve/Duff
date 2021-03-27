#pragma once

#include <QAbstractItemModel>
#include <QVector>

class ResultModel : public QAbstractItemModel
{
	Q_OBJECT

	struct Node
	{
		Node(Node* parent, const QMap<Qt::ItemDataRole, QVariant>& data);
		~Node();

		int parentRow() const;

		Node* _parent;
		QVector<Node*> _children;
		QMap<Qt::ItemDataRole, QVariant> _data;
	};

public:
	explicit ResultModel(QObject* parent = nullptr);
	~ResultModel();

	QModelIndex index(int row, int column, const QModelIndex& parentIndex = QModelIndex()) const override;
	QModelIndex parent(const QModelIndex& childIndex) const override;

	int rowCount(const QModelIndex& parentIndex = QModelIndex()) const override;
	int columnCount(const QModelIndex& parentIndex = QModelIndex()) const override;

	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
	bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

	Qt::ItemFlags flags(const QModelIndex& index) const override;

	void clear();
	void addPath(const QString& hash, const QString& filePath);
	QStringList selectedPaths() const;
	void removePath(const QString& filePath);

private:
	Node* _root = nullptr;
};

