#include "ResultModel.hpp"
#include "ResultModel.hpp"

#include <QApplication>
#include <QColor>
#include <QPalette>
#include <QQueue>

inline Node* indexToNode(const QModelIndex& index)
{
	return static_cast<Node*>(index.internalPointer());
}

class Node
{
public:
	Node(Node* parent, const QMap<Qt::ItemDataRole, QVariant>& data) :
		_parent(parent),
		_data(data)
	{
	}

	~Node()
	{
		qDeleteAll(_children);
		qDebug() << _data[Qt::DisplayRole];
	}

	const Node* parent() const
	{
		return _parent;
	}

	Node* childAt(int index) const
	{
		return _children.at(index);
	}

	Node* takeChild(int index)
	{
		return _children.takeAt(index);
	}

	QVector<Node*> takeChildren(const std::function<bool(Node*)>& lambda)
	{
		QVector<Node*> result;

		for (int i = 0; i < childCount(); ++i)
		{
			if (lambda(childAt(i)))
			{
				result.append(takeChild(i));
			}
		}

		return result;
	}

	void appendChild(Node* child)
	{
		_children.append(child);
	}

	QVector<Node*> findChildren(const std::function<bool(Node*)>& lambda) const
	{
		QVector<Node*> results;
		QQueue<Node*> queue;
		queue.enqueue(const_cast<Node*>(this));

		while (!queue.empty())
		{
			Node* node = queue.dequeue();

			for (Node* child : node->_children)
			{
				queue.enqueue(child);
			}

			if (lambda(node))
			{
				results.push_back(node);
			}
		}

		return results;
	}

	Node* findChild(const std::function<bool(Node*)>& lambda) const
	{
		auto children = findChildren(lambda);
		Q_ASSERT(children.size() <= 1);
		return children.first();
	}

	int parentRow() const
	{
		return _parent ? _parent->_children.indexOf(this) : 0;
	}

	int childCount() const
	{
		return _children.size();
	}

	bool hasChildren() const
	{
		return !_children.isEmpty();
	}

	QVariant& data(Qt::ItemDataRole role)
	{
		return _data[role];
	}

private:
	const Node* _parent;
	QVector<Node*> _children;
	QMap<Qt::ItemDataRole, QVariant> _data;
};


ResultModel::ResultModel(QObject *parent) :
	QAbstractItemModel(parent),
	_root(new Node(nullptr, { { Qt::DisplayRole, "root" } }))
{
}

ResultModel::~ResultModel()
{
	delete _root;
}

QModelIndex ResultModel::index(int row, int column, const QModelIndex& parentIndex) const
{
	if (!hasIndex(row, column, parentIndex))
	{
		return QModelIndex();
	}

	Node* parent= parentIndex.isValid() ? indexToNode(parentIndex) : _root;
	Node* child = parent ->childAt(row);
	return child ? createIndex(row, column, child) : QModelIndex();
}

QModelIndex ResultModel::parent(const QModelIndex& childIndex) const
{
	if (!childIndex.isValid())
	{
		return QModelIndex();
	}

	const Node* parentNode = indexToNode(childIndex)->parent();

	return parentNode != _root ?
		createIndex(parentNode->parentRow(), 0, parentNode) :
		QModelIndex();
}

int ResultModel::rowCount(const QModelIndex& parentIndex) const
{
	return parentIndex.isValid() ? 
		indexToNode(parentIndex)->childCount() :
		_root->childCount();
}

int ResultModel::columnCount(const QModelIndex&) const
{
	return 2;
}

QVariant ResultModel::data(const QModelIndex& index, int role) const
{
	if (!index.isValid())
	{
		return QVariant();
	}

	Node* item = indexToNode(index);
	bool topLevel = item->parent() == _root;
	bool hashCell = index.column() == 0 && topLevel;
	bool pathCell = index.column() == 1 && !topLevel;

	if (role == Qt::DisplayRole)
	{
		if (hashCell || pathCell)
		{
			return item->data(Qt::DisplayRole);
		}
	}

	if (role == Qt::BackgroundRole)
	{
		if (!topLevel)
		{
			static const QPalette& palette = QApplication::palette();
			return index.row() % 2 ? palette.dark() : palette.midlight();
		}
	}

	if (role == Qt::CheckStateRole)
	{
		if (pathCell)
		{
			return item->data(Qt::CheckStateRole);
		}
	}

	return QVariant();
}

bool ResultModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
	Node* item = indexToNode(index);
	bool topLevel = item->parent() == _root;
	bool pathCell = index.column() == 1 && !topLevel;

	if (role == Qt::CheckStateRole && pathCell)
	{
		item->data(Qt::CheckStateRole) = value;
		emit dataChanged(index, index);
		return true;
	}
	
	return false;
}

QVariant ResultModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
	{
		return QVariant();
	}

	switch (section)
	{
		case 0:
			return "Hash";
		case 1:
			return "Path";
	}

	return QVariant();
}

Qt::ItemFlags ResultModel::flags(const QModelIndex& index) const
{
	if (index.column() != 1)
	{
		return Qt::ItemIsEnabled;
	}

	return Qt::ItemIsEnabled | Qt::ItemIsUserCheckable;
}

void ResultModel::clear()
{
	beginResetModel();
	delete _root;
	_root = new Node(nullptr, { { Qt::DisplayRole, "root" } });
	endResetModel();
}

void ResultModel::addPath(const QString& hash, const QString& filePath)
{
	if (!_root->hasChildren())
	{
		beginInsertRows(QModelIndex(), 0, 1);
		auto hashNode = new Node(_root, { { Qt::DisplayRole, hash } });
		hashNode->appendChild(
			new Node(hashNode, { { Qt::DisplayRole, filePath }, { Qt::CheckStateRole, false } }));
		_root->appendChild(hashNode);
		endInsertRows();
	}
	else
	{
		Node* hashNode = _root->findChild([&hash](Node* node)
		{
			return node->data(Qt::DisplayRole).toString() == hash;
		});

		if (hashNode)
		{
			hashNode->appendChild(
				new Node(hashNode, { { Qt::DisplayRole, filePath }, { Qt::CheckStateRole, false } }));
		}
	}
}

QStringList ResultModel::selectedPaths() const
{
	QStringList results;

	auto isChecked = [&](Node* node)
	{
		return node->data(Qt::CheckStateRole) == Qt::CheckState::Checked;
	};

	for (Node* node : _root->findChildren(isChecked))
	{
		results.append(node->data(Qt::DisplayRole).toString());
	}

	return results;
}

void ResultModel::removePath(const QString& filePath)
{
	beginResetModel();

	auto pathEquals = [&](Node* node)
	{
		return node->data(Qt::DisplayRole).toString() == filePath;
	};

	for (int i = 0; i < _root->childCount(); ++i)
	{
		Node* hashNode = _root->childAt(i);

		auto result = hashNode->takeChildren(pathEquals);

		qDeleteAll(result);

		if (hashNode->childCount() < 2)
		{
			delete _root->takeChild(i);
		}
	}

	endResetModel();
}
