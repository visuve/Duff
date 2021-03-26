#include "ResultModel.hpp"

ResultModel::Node::Node(Node* parent, const QVector<QVariant>& data) :
	_parent(parent),
	_data(data)
{
}

ResultModel::Node::~Node()
{
	qDeleteAll(_children);
	qDebug() << _data[0];
}

int ResultModel::Node::parentRow() const
{
	return _parent ?
		_parent->_children.indexOf(const_cast<Node*>(this)) :
		0;
}

ResultModel::ResultModel(QObject *parent) :
	QAbstractItemModel(parent),
	_root(new Node(nullptr, { "root" }))
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

	Node* parentNode = parentIndex.isValid() ?
		static_cast<Node*>(parentIndex.internalPointer()) :
		_root;

	Node* childItem = parentNode->_children.at(row);

	return childItem ?
		createIndex(row, column, childItem) :
		QModelIndex();
}

QModelIndex ResultModel::parent(const QModelIndex& childIndex) const
{
	if (!childIndex.isValid())
	{
		return QModelIndex();
	}

	Node* parentNode = static_cast<Node*>(childIndex.internalPointer())->_parent;

	return parentNode != _root ?
		createIndex(parentNode->parentRow(), 0, parentNode) :
		QModelIndex();
}

int ResultModel::rowCount(const QModelIndex& parentIndex) const
{
	return parentIndex.isValid() ?
		static_cast<Node *>(parentIndex.internalPointer())->_children.size() :
		_root->_children.size();
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

	Node* item = static_cast<Node*>(index.internalPointer());
	bool topLevel = item->_parent == _root;
	bool hashCell = index.column() == 0 && topLevel;
	bool pathCell = index.column() == 1 && !topLevel;

	if (role == Qt::DisplayRole)
	{
		if (hashCell || pathCell)
		{
			return item->_data[0];
		}
	}

	if (role == Qt::CheckStateRole)
	{
		if (pathCell)
		{
			return item->_data[1].toBool() ? Qt::Checked : Qt::Unchecked;
		}
	}

	return QVariant();
}

bool ResultModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
	Node* item = static_cast<Node*>(index.internalPointer());
	bool topLevel = item->_parent == _root;
	bool pathCell = index.column() == 1 && !topLevel;

	if (role == Qt::CheckStateRole && pathCell)
	{
		item->_data[1] = value == Qt::Checked;
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
	_root = new Node(nullptr, { "root" });
	endResetModel();
}

void ResultModel::addItem(const QString& hash, const QString& filePath)
{
	if (_root->_children.size() <= 0)
	{
		beginInsertRows(QModelIndex(), 0, 1);
		auto hashNode = new Node(_root, { hash });
		hashNode->_children.append(new Node(hashNode, { filePath, false }));
		_root->_children.append(hashNode);
		endInsertRows();
		return;
	}

	for (Node* hashNode : _root->_children)
	{
		if (hashNode->_data[0] == hash)
		{
			hashNode->_children.append(new Node(hashNode, { filePath, false }));
			break;
		}
	}
}

QStringList ResultModel::selectedPaths() const
{
	QStringList results;

	for (Node* hashNode : _root->_children)
	{
		for (Node* pathNode : hashNode->_children)
		{
			if (pathNode->_data[1].toBool())
			{
				results.append(pathNode->_data[0].toString());
			}
		}
	}

	return results;
}

void ResultModel::removePath(const QString& filePath)
{
	beginResetModel();

	for (int i = 0; i < _root->_children.size();)
	{
		Node* hashNode = _root->_children[i];

		for (int j = 0; j < hashNode->_children.size();)
		{
			Node* pathNode = hashNode->_children[j];

			if (pathNode->_data[0].toString() != filePath)
			{
				++j;
				continue;
			}

			delete hashNode->_children.takeAt(j);
		}

		if (!hashNode->_children.isEmpty())
		{
			++i;
			continue;
		}

		delete _root->_children.takeAt(i);
	}

	endResetModel();
}
