#pragma  once

template<typename T>
class optional
{
public:
	optional():haved(false){}
	optional(const T& _val):haved(true),val(_val){}
	optional(const optional& _opt):haved(_opt.haved), val(_opt.val){}
	~optional() {}

	optional& operator= (const T&_val)
	{
		val = _val;
		haved = true;

		return *this;
	}

	bool has_value()const {return haved;}

	const T& value()const { return val; }

	void reset() { haved = false; }
private:
	bool	haved;
	T		val;
};