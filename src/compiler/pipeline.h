/*
** GB BASIC
**
** Copyright (C) 2023-2025 Tony Wang, all rights reserved
**
** For the latest info, see https://paladin-t.github.io/kits/gbb/
*/

#ifndef __PIPELINE_H__
#define __PIPELINE_H__

#include "../gbbasic.h"
#include "../utils/assets.h"

/*
** {===========================================================================
** Pipeline
*/

namespace GBBASIC {

class Pipeline : public virtual Object {
public:
	typedef std::shared_ptr<Pipeline> Ptr;

	struct Resource {
		typedef std::vector<Resource> Array;

		int bank = 0;
		int address = 0;
		int size = 0;

		Resource();
		Resource(int b, int a, int s);
	};

	struct Size {
	private:
		int _font = 0;
		int _code = 0;
		int _tiles = 0;
		int _map = 0;
		int _music = 0;
		int _sfx = 0;
		int _actor = 0;
		int _scene = 0;
		int _borderResources = 0;

	public:
		Size();

		Size &operator += (const Size &other);

		int font(void) const;
		void addFont(int val);
		int code(void) const;
		void addCode(int val);
		int tiles(void) const;
		void addTiles(int val);
		int map(void) const;
		void addMap(int val);
		int music(void) const;
		void addMusic(int val);
		int sfx(void) const;
		void addSfx(int val);
		int actor(void) const;
		void addActor(int val);
		int scene(void) const;
		void addScene(int val);
		int borderResources(void) const;
		void addBorderResources(int val);

		int total(void) const;

		std::string toString(int indent = 2) const;
	};

	typedef std::function<bool(const std::string &/* where */, int* /* bank */, int* /* address */, int* /* size */)> CodeLocationHandler;

	typedef std::function<void(const std::string & /* msg */, AssetsBundle::Categories /* category */)> PrintHandler;
	typedef std::function<void(const std::string & /* msg */, bool /* isWarning */, AssetsBundle::Categories /* category */, int /* page */)> ErrorHandler;

public:
	GBBASIC_CLASS_TYPE('P', 'I', 'P', 'E')

	virtual unsigned compatibility(void) const = 0;
	virtual void compatibility(unsigned val) = 0;

	virtual int romMaxSize(void) const = 0;
	virtual void romMaxSize(int val) = 0;

	virtual int startBank(void) const = 0;
	virtual void startBank(int val) = 0;

	virtual int bankSize(void) const = 0;
	virtual void bankSize(int val) = 0;

	virtual int startAddress(void) const = 0;
	virtual void startAddress(int val) = 0;

	virtual int* bank(void) const = 0;
	virtual void bank(int* val) = 0;

	virtual int* addressCursor(void) const = 0;
	virtual void addressCursor(int* val) = 0;

	virtual Bytes::Ptr bytes(void) const = 0;
	virtual void bytes(Bytes::Ptr val) = 0;

	virtual CodeLocationHandler codeLocationHandler(void) const = 0;
	virtual void codeLocationHandler(CodeLocationHandler val) = 0;

	virtual const Size &effectiveSize(void) const = 0;
	virtual Size &effectiveSize(void) = 0;

	virtual void open(void) = 0;
	virtual void close(void) = 0;

	virtual ActorAssets::Entry::PlayerBehaviourCheckingHandler isPlayerBehaviour(void) const = 0;
	virtual void isPlayerBehaviour(ActorAssets::Entry::PlayerBehaviourCheckingHandler val) = 0;

	virtual PrintHandler onPrint(void) const = 0;
	virtual void onPrint(PrintHandler val) = 0;

	virtual ErrorHandler onError(void) const = 0;
	virtual void onError(ErrorHandler val) = 0;

	/**
	 * @brief Pipes all source assets to target bytes.
	 */
	virtual bool pipe(AssetsBundle::ConstPtr assets, bool includeUnused, bool optimizeAssets) = 0;

	/**
	 * @brief Touches an asset page as inuse.
	 */
	virtual bool touch(AssetsBundle::ConstPtr assets, AssetsBundle::Categories category, int page, bool incRef) = 0;
	/**
	 * @brief Looks for the specific asset's location in ROM.
	 */
	virtual bool lookup(AssetsBundle::Categories category, int page, Resource::Array &locations) const = 0;
	/**
	 * @brief Looks for the specific asset's ref count.
	 */
	virtual bool lookup(AssetsBundle::Categories category, int page, int &refCount) const = 0;

	/**
	 * @brief Prompts for unused assets, etc.
	 */
	virtual void prompt(AssetsBundle::ConstPtr assets) = 0;

	static Pipeline* create(bool useWorkQueue, bool lessConsoleOutput);
	static void destroy(Pipeline* ptr);
};

}

/* ===========================================================================} */

#endif /* __PIPELINE_H__ */
