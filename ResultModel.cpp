#include "ResultModel.hpp"

#include <QDebug>
#include <QFile>
#include <QQueue>
#include <QVector>

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

	QVector<Node*> takeChildren(const std::function<bool(const Node*)>& predicate)
	{
		QVector<Node*> result;

		int i = childCount();

		while (i--)
		{
			if (predicate(childAt(i)))
			{
				result.append(takeChild(i));
			}
		}

		return result;
	}

	Node* appendChild(const QMap<Qt::ItemDataRole, QVariant>& data)
	{
		Node* child = new Node(this, data);
		_children.append(child);
		return child;
	}

	QVector<Node*> findChildren(const std::function<bool(const Node*)>& lambda) const
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

	Node* findChild(const std::function<bool(const Node*)>& lambda) const
	{
		auto children = findChildren(lambda);
		return children.isEmpty() ? nullptr : children.first();
	}

	int parentRow() const
	{
		return _parent ? _parent->_children.indexOf(const_cast<Node*>(this)) : 0;
	}

	int childCount() const
	{
		return _children.size();
	}

	bool hasChildren() const
	{
		return !_children.isEmpty();
	}

	QVariant value(Qt::ItemDataRole role, const QVariant& defaultValue = QVariant()) const
	{
		return _data.value(role, defaultValue);
	}

	void setValue(Qt::ItemDataRole role, const QVariant& value)
	{
		_data[role] = value;
	}

	QString displayString() const
	{
		return _data.value(Qt::DisplayRole).toString();
	}

	bool isChecked() const
	{
		return _data.value(Qt::CheckStateRole) == Qt::CheckState::Checked;
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

	const Node* parent= parentIndex.isValid() ? indexToNode(parentIndex) : _root;
	const Node* child = parent ->childAt(row);
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

	const Node* item = indexToNode(index);
	bool topLevel = item->parent() == _root;
	bool hashCell = index.column() == 0 && topLevel;
	bool pathCell = index.column() == 1 && !topLevel;

	if (role == Qt::DisplayRole)
	{
		if (hashCell || pathCell)
		{
			return item->value(Qt::DisplayRole);
		}
	}

	if (role == Qt::CheckStateRole)
	{
		if (pathCell)
		{
			return item->value(Qt::CheckStateRole);
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
		if (value == Qt::CheckState::Checked)
		{
			++_selectedCount;
		}
		else if (value == Qt::CheckState::Unchecked)
		{
			--_selectedCount;
		}
		else
		{
			qDebug() << "Invalid check state value:" << value;
			return false;
		}

		item->setValue(Qt::CheckStateRole, value);
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
	_totalCount = 0;
	_selectedCount = 0;
	endResetModel();
}

void ResultModel::addPath(const QString& hash, const QString& filePath)
{
	const auto hashEquals = [&](const Node* node)
	{
		return node->displayString() == hash;
	};

	Node* hashNode = _root->findChild(hashEquals);

	if (!hashNode)
	{
		int newHashRow = _root->childCount();
		beginInsertRows(QModelIndex(), newHashRow, newHashRow);
		hashNode = _root->appendChild({ { Qt::DisplayRole, hash } });
		hashNode->appendChild({ { Qt::DisplayRole, filePath }, { Qt::CheckStateRole, Qt::Unchecked } });
		++_totalCount;
		endInsertRows();
	}
	else
	{
		QModelIndex hashIndex = createIndex(hashNode->parentRow(), 0, hashNode);
		int newPathRow = hashNode->childCount();
		beginInsertRows(hashIndex, newPathRow, newPathRow);
		hashNode->appendChild({ { Qt::DisplayRole, filePath }, { Qt::CheckStateRole, Qt::Unchecked } });
		++_totalCount;
		endInsertRows();
	}
}

QStringList ResultModel::selectedPaths() const
{
	QStringList results;

	const auto isChecked = [&](const Node* node)
	{
		return node->isChecked();
	};

	for (const Node* node : _root->findChildren(isChecked))
	{
		results.append(node->displayString());
	}

	return results;
}

int ResultModel::totalCount() const
{
	return _totalCount;
}

int ResultModel::selectedCount() const
{
	return _selectedCount;
}


void ResultModel::prune(const std::function<bool(const Node*)>& predicate)
{
	int i = _root->childCount();

	beginResetModel();

	while (i--)
	{
		Node* hashNode = _root->childAt(i);

		for (const Node* pathNode : hashNode->takeChildren(predicate))
		{
			if (pathNode->isChecked())
			{
				--_selectedCount;
			}

			--_totalCount;

			delete pathNode;
		}

		// If a hash has one of fewer instances remove it from the model
		// The point of the application is to find duplicates
		if (hashNode->childCount() < 2)
		{
			// Check if the hash node has a lone child
			if (hashNode->childCount() == 1)
			{
				const Node* lone = hashNode->childAt(0);

				if (lone->isChecked())
				{
					--_selectedCount;
				}

				--_totalCount;
			}

			// Delete the hash node itself
			// Which will also delete any remaining children. see Node dtor
			delete _root->takeChild(i);
		}
	}

	endResetModel();
}

void ResultModel::removePath(const QString& filePath)
{
	const auto pathEquals = [&](const Node* node)
	{
		return node->displayString() == filePath;
	};

	prune(pathEquals);
}

void ResultModel::removeInexistentPaths()
{
	const auto missing = [](const Node* node)
	{
		const QString filePath = node->displayString();
		return !QFile::exists(filePath);
	};

	prune(missing);
}
