#include "ResultModel.hpp"

ResultModel::Node::Node(Node* parent, const QVector<QVariant>& data) :
	_parent(parent),
	_data(data)
{
}

ResultModel::Node::~Node()
{
	qDeleteAll(_children);
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
	auto alpha = new Node(_root, { "59A621333B9D06D35B70BB501B5E02CCAFD7B31B" });
	alpha->_children.append(new Node(alpha, { "foo.txt", false }));
	alpha->_children.append(new Node(alpha, { "bar.txt", false }));
	alpha->_children.append(new Node(alpha, { "foobar.txt", false }));
	alpha->_children.append(new Node(alpha, { "barfoo.txt", false }));

	auto bravo = new Node(_root, { "A23D723D5C996508FC0BED38E9726916D0A3BB1A" });
	bravo->_children.append(new Node(bravo, { "x.txt", false }));
	bravo->_children.append(new Node(bravo, { "y.txt", false }));
	bravo->_children.append(new Node(bravo, { "z.txt", false }));
	bravo->_children.append(new Node(bravo, { "w.txt", false }));

	_root->_children.append(alpha);
	_root->_children.append(bravo);
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

	return Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsUserCheckable;
}

void ResultModel::clear()
{
	beginResetModel();
	delete _root;
	_root = new Node(nullptr, { "root" });
	endResetModel();
}
