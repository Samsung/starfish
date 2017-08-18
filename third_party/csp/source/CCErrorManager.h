/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

/*
 * CCErrorManager class function header file
 * (Subcomponent of CCError Implementation)
 *
 * \author Young-il Choi <duddlf.choi@samsung.com>
 * \date 2006-07-28
 */




#if (CONFIG_TLS_ERROR == 0)

class CCErrorManager
{
private:
	struct CTError
	{
		long id;
		int num;
	};

	CCList	m_list;
	CCMutex	m_listLock;

public:
	CCErrorManager(void) {}
	virtual ~CCErrorManager(void) {}

	bool Create(void);
	virtual void Destroy(void);

	bool FlagCreate() { return m_listLock.FlagCreate(); }

	bool AddItem();
	void DeleteItem();
	
	bool Set(int num);
	int Num(void);
};

#endif




